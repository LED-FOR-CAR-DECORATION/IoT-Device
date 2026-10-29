#include "OtaManager.h"
#include "BleManager.h"
#include "WifiManager.h"
#include "config.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <mbedtls/sha256.h>
#include <time.h>
#include <algorithm>

#if __has_include("ota_config.h")
#include "ota_config.h"
#else
#define OTA_API_BASE ""
#define OTA_ROOT_CA ""
#endif

// Referenced by sendInfo so the hardware marker is retained by the linker.
extern "C" const char carledHardwareMarker[] = "CARLED_HW:" HARDWARE_REV;
extern "C" const char carledFirmwareMarker[] = "CARLED_FW:" FIRMWARE_VERSION;
static String deviceId() {
    uint8_t mac[6]; esp_efuse_mac_get_default(mac);
    char text[13]; snprintf(text,sizeof(text),"%02X%02X%02X%02X%02X%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return String(text);
}
static void emit(const char* state,int progress,const String& id,const char* error="") {
    JsonDocument doc; doc["event"]="ota_status"; doc["state"]=state;
    doc["progress"]=progress; doc["request_id"]=id; doc["error"]=error;
    String json; serializeJson(doc,json); BleManager::getInstance().sendStatusNotification(json);
}
OtaManager& OtaManager::getInstance() { static OtaManager manager; return manager; }
void OtaManager::rotateNonce() {
    uint8_t bytes[16]; esp_fill_random(bytes,sizeof(bytes)); char hex[33];
    for (int i=0;i<16;++i) snprintf(hex+i*2,3,"%02x",bytes[i]);
    nonce=hex;
}
void OtaManager::earlyBoot() {
    // App-level recovery with a stock bootloader. Recovery before setup requires
    // an IDF bootloader compiled with CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE.
    Preferences prefs; if (!prefs.begin("ota_state",false)) return;
    uint32_t target=prefs.getUInt("target",0);
    const auto* running=esp_ota_get_running_partition();
    if (target && running && target==running->address) {
        if (prefs.getBool("attempted",false)) {
            uint32_t previous=prefs.getUInt("previous",0);
            auto it=esp_partition_find(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_ANY,nullptr);
            while (it) {
                const auto* p=esp_partition_get(it);
                if (p->address==previous && p->address!=target && esp_ota_set_boot_partition(p)==ESP_OK) {
                    prefs.clear(); prefs.end(); esp_partition_iterator_release(it); ESP.restart(); return;
                }
                it=esp_partition_next(it);
            }
        } else prefs.putBool("attempted",true);
    } else if (target) prefs.clear();
    prefs.end();
}
void OtaManager::init() { rotateNonce(); healthySince=millis(); }
void OtaManager::sendInfo(const String& id) {
    JsonDocument doc; doc["event"]="ota_info"; doc["request_id"]=id;
    doc["device_id"]=deviceId(); doc["version"]=carledFirmwareMarker+10; doc["hardware"]=carledHardwareMarker+10;
    doc["nonce"]=nonce; doc["wifi"]=WifiManager::getInstance().isConnected();
    doc["configured"]=String(OTA_API_BASE).startsWith("https://") && strlen(OTA_ROOT_CA)>0;
    doc["busy"]=busy.load();
    doc["healthy"]=healthySince==0;
    String json; serializeJson(doc,json); BleManager::getInstance().sendStatusNotification(json);
}
void OtaManager::start(const String& capability,const String& id) {
    if (busy.exchange(true)) { emit("error",0,id,"ota_busy"); return; }
    if (capability.length()!=64 || id.length()>36) { busy=false; emit("error",0,id,"invalid_request"); return; }
    for (unsigned i=0;i<capability.length();++i) if (!isxdigit(capability[i])) { busy=false; emit("error",0,id,"invalid_token"); return; }
    if (!String(OTA_API_BASE).startsWith("https://") || strlen(OTA_ROOT_CA)==0) { busy=false; emit("error",0,id,"ota_not_configured"); return; }
    if (!WifiManager::getInstance().isConnected()) { busy=false; emit("error",0,id,"wifi_required"); return; }
    token=capability; requestId=id; sessionNonce=nonce; rotateNonce();
    progress=0; result=0; lastProgress=-1;
    if (xTaskCreatePinnedToCore(worker,"https_ota",12288,this,1,nullptr,0)!=pdPASS) {
        token=""; busy=false; emit("error",0,id,"out_of_memory");
    } else emit("starting",0,id);
}
void OtaManager::worker(void* arg) { static_cast<OtaManager*>(arg)->run(); vTaskDelete(nullptr); }
void OtaManager::fail(const char* code) { snprintf(failure,sizeof(failure),"%s",code); result.store(1); }
void OtaManager::run() {
    if (time(nullptr)<1700000000) {
        configTime(0,0,"pool.ntp.org","time.google.com"); unsigned long started=millis();
        while (time(nullptr)<1700000000 && millis()-started<15000) delay(100);
        if (time(nullptr)<1700000000) { fail("clock_sync_failed"); return; }
    }
    WiFiClientSecure tls; tls.setCACert(OTA_ROOT_CA); tls.setHandshakeTimeout(15);
    HTTPClient http; http.setConnectTimeout(15000); http.setTimeout(15000);
    http.useHTTP10(true);
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    String path=String(OTA_API_BASE)+"/api/firmware/downloads/"+token;
    if (!http.begin(tls,path+"/manifest")) { fail("https_failed"); return; }
    int status=http.GET();
    if (status!=200 || http.getSize()==0 || http.getSize()>2048) { http.end(); fail("manifest_rejected"); return; }
    // Jackson responses need not carry Content-Length. HTTP/1.0 avoids chunk
    // framing; HTTPClient reads until close when the JSON size is unknown.
    String manifest=http.getString(); http.end();
    if (manifest.length()>2048) { fail("manifest_rejected"); return; }
    JsonDocument doc; auto parsed=deserializeJson(doc,manifest);
    JsonObject data=doc["data"]; String expectedHash=data["sha256"]|""; String version=data["version"]|"";
    size_t size=data["size"]|0;
    if (parsed || String(data["deviceId"]|"")!=deviceId() || String(data["nonce"]|"")!=sessionNonce
            || String(data["hardwareRevision"]|"")!=HARDWARE_REV || version==FIRMWARE_VERSION || expectedHash.length()!=64) {
        fail("manifest_mismatch"); return;
    }
    const auto* partition=esp_ota_get_next_update_partition(nullptr);
    if (!partition || size<288 || size>partition->size) { fail("image_too_large"); return; }
    if (!http.begin(tls,path+"/binary")) { fail("https_failed"); return; }
    status=http.GET();
    if (status!=200 || http.getSize()!=(int)size) { http.end(); fail("download_rejected"); return; }
    esp_ota_handle_t handle;
    if (esp_ota_begin(partition,size,&handle)!=ESP_OK) { http.end(); fail("flash_begin_failed"); return; }
    mbedtls_sha256_context sha; mbedtls_sha256_init(&sha); mbedtls_sha256_starts_ret(&sha,0);
    uint8_t buffer[1024]; size_t received=0; unsigned long lastData=millis(),startTime=millis();
    bool failed=false; const char* error="download_interrupted"; auto stream=http.getStreamPtr();
    while (received<size) {
        int available=stream->available();
        if (millis()-startTime>300000) { failed=true; break; }
        if (available>0) {
            size_t count=stream->readBytes(buffer,std::min(sizeof(buffer),std::min((size_t)available,size-received)));
            if (!count) { failed=true; break; }
            if (received==0) {
                while (count<288 && millis()-lastData<15000) {
                    size_t extra=stream->readBytes(buffer+count,288-count); if (!extra) break; count+=extra;
                }
                if (count<288 || buffer[0]!=0xe9) {
                    failed=true; error="invalid_esp_image"; break;
                }
            }
            if (mbedtls_sha256_update_ret(&sha,buffer,count)!=0 || esp_ota_write(handle,buffer,count)!=ESP_OK) {
                failed=true; error="flash_write_failed"; break;
            }
            received+=count; lastData=millis(); progress=(received*100)/size;
        } else if (!http.connected() || millis()-lastData>15000 || millis()-startTime>300000) { failed=true; break; }
        delay(1);
    }
    uint8_t digest[32]; mbedtls_sha256_finish_ret(&sha,digest); mbedtls_sha256_free(&sha); http.end();
    char actual[65]; for (int i=0;i<32;++i) snprintf(actual+i*2,3,"%02x",digest[i]);
    if (failed || received!=size || expectedHash!=actual) { esp_ota_abort(handle); fail(failed?error:"checksum_mismatch"); return; }
    if (esp_ota_end(handle)!=ESP_OK) { fail("invalid_esp_image"); return; }
    Preferences prefs; if (!prefs.begin("ota_state",false)) { fail("state_write_failed"); return; }
    bool saved=prefs.putUInt("previous",esp_ota_get_running_partition()->address)>0
            && prefs.putUInt("target",partition->address)>0 && prefs.putBool("attempted",false)>0;
    prefs.end();
    if (!saved || esp_ota_set_boot_partition(partition)!=ESP_OK) { fail("boot_switch_failed"); return; }
    result.store(2);
}
void OtaManager::update() {
    if (!busy && healthySince && millis()-healthySince>15000) {
        esp_ota_img_states_t state;
        if (esp_ota_get_state_partition(esp_ota_get_running_partition(),&state)==ESP_OK && state==ESP_OTA_IMG_PENDING_VERIFY)
            esp_ota_mark_app_valid_cancel_rollback();
        Preferences prefs; if (prefs.begin("ota_state",false)) { prefs.clear(); prefs.end(); }
        healthySince=0;
    }
    if (!busy) return;
    int finished=result.load();
    if (finished==1) { emit("error",progress,requestId,failure); token=""; busy=false; result=0; }
    else if (finished==2) { emit("rebooting",100,requestId); delay(700); ESP.restart(); }
    else if (progress.load()/5!=lastProgress/5 || lastProgress<0) { lastProgress=progress.load(); emit("downloading",lastProgress,requestId); }
}
