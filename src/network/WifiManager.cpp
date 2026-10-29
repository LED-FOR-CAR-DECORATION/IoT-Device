#include "WifiManager.h"
#include "BleManager.h"
#include "OtaManager.h"
#include "config.h"
#include "core/StateManager.h"
#include "effects/EffectManager.h"
#include "storage/StorageManager.h"
#include "utils/Logger.h"
#include <ArduinoJson.h>
#include <mbedtls/base64.h>
#include "secrets.example.h"
#if __has_include("secrets.h")
#include "secrets.h"
#endif

#define TAG "WIFI"
#define RECONNECT_INTERVAL_MS 10000
#define PREF_WIFI_NS "wifi_cfg"

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
<title>Car LED Matrix Controller</title>
<style>
  :root {
    --bg: #0f111a;
    --card: #181b2a;
    --accent: #00d2ff;
    --accent-grad: linear-gradient(135deg, #00d2ff 0%, #7928ca 100%);
    --text: #ffffff;
    --text-muted: #8c93ad;
    --border: #252a40;
    --success: #00f076;
    --danger: #ff3366;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
  body { background: var(--bg); color: var(--text); padding: 16px; min-height: 100vh; }
  .container { max-width: 480px; margin: 0 auto; display: flex; flex-direction: column; gap: 16px; }
  header { text-align: center; padding: 12px 0; }
  header h1 { font-size: 22px; font-weight: 800; background: var(--accent-grad); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
  .badge { display: inline-block; padding: 4px 10px; border-radius: 20px; font-size: 12px; background: rgba(0, 240, 118, 0.15); color: var(--success); margin-top: 6px; }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 14px; padding: 16px; }
  .card h2 { font-size: 16px; margin-bottom: 12px; color: var(--accent); display: flex; align-items: center; gap: 8px; }
  .preview-wrap { text-align: center; margin: 12px 0; background: #000; border-radius: 10px; padding: 10px; border: 2px solid #222; }
  canvas { width: 100%; max-width: 320px; height: auto; aspect-ratio: 2/1; image-rendering: pixelated; border-radius: 6px; box-shadow: 0 0 16px rgba(0,210,255,0.2); }
  .btn { width: 100%; padding: 12px; border-radius: 10px; border: none; font-weight: 700; font-size: 14px; cursor: pointer; transition: 0.2s; background: var(--accent-grad); color: #fff; margin-top: 8px; }
  .btn:active { transform: scale(0.98); }
  .btn-outline { background: transparent; border: 1px solid var(--accent); color: var(--accent); }
  .btn-danger { background: var(--danger); color: #fff; }
  .controls-row { display: flex; gap: 8px; margin-top: 8px; }
  .slider-group { margin: 10px 0; }
  .slider-group label { display: flex; justify-content: space-between; font-size: 13px; color: var(--text-muted); margin-bottom: 4px; }
  input[type=range] { width: 100%; accent-color: var(--accent); }
  .file-input-wrap { position: relative; overflow: hidden; display: inline-block; width: 100%; }
  .file-input-wrap input[type=file] { position: absolute; left: 0; top: 0; opacity: 0; width: 100%; height: 100%; cursor: pointer; }
  .file-btn { display: block; text-align: center; padding: 12px; background: rgba(0,210,255,0.1); border: 1px dashed var(--accent); border-radius: 10px; color: var(--accent); font-weight: 600; font-size: 13px; }
  .grid-modes { display: grid; grid-template-columns: repeat(2, 1fr); gap: 8px; margin-top: 8px; }
  .mode-btn { padding: 10px; background: #202438; border: 1px solid var(--border); border-radius: 8px; color: #fff; font-size: 13px; font-weight: 600; cursor: pointer; text-align: center; }
  .mode-btn.active { border-color: var(--accent); background: rgba(0,210,255,0.15); color: var(--accent); }
  .status-toast { padding: 10px; border-radius: 8px; font-size: 13px; text-align: center; margin-top: 10px; display: none; }
  .status-ok { background: rgba(0,240,118,0.15); color: var(--success); border: 1px solid var(--success); }
  .status-err { background: rgba(255,51,102,0.15); color: var(--danger); border: 1px solid var(--danger); }
  input[type=text], input[type=password] { width: 100%; padding: 10px; background: #0f111a; border: 1px solid var(--border); border-radius: 8px; color: #fff; font-size: 14px; margin-bottom: 8px; }
</style>
</head>
<body>
<div class="container">
  <header>
    <h1>🚗 Car LED Matrix</h1>
    <div class="badge" id="netBadge">Wi-Fi Connected</div>
  </header>

  <!-- Flow 2: Image Projection -->
  <div class="card">
    <h2>🖼️ Import Ảnh Lên Bảng LED (64x32)</h2>
    <div class="file-input-wrap">
      <div class="file-btn" id="fileBtnLabel">📁 Chọn ảnh từ điện thoại (JPG / PNG)</div>
      <input type="file" id="imageInput" accept="image/*">
    </div>

    <div class="preview-wrap">
      <canvas id="previewCanvas" width="64" height="32"></canvas>
    </div>

    <div class="slider-group">
      <label><span>Độ sáng ảnh (Brightness)</span><span id="imgBriVal">100%</span></label>
      <input type="range" id="imgBriSlider" min="50" max="200" value="100">
    </div>

    <div class="slider-group">
      <label><span>Tương phản (Contrast)</span><span id="imgConVal">100%</span></label>
      <input type="range" id="imgConSlider" min="50" max="200" value="100">
    </div>

    <button class="btn" id="uploadBtn">🚀 Chiếu Lên Bảng LED</button>
    <div class="status-toast" id="uploadStatus"></div>
  </div>

  <!-- Realtime Controls -->
  <div class="card">
    <h2>🎛️ Điều Khiển Hiệu Ứng</h2>
    <div class="controls-row">
      <button class="btn" id="pwrBtn">Bật / Tắt Đèn</button>
    </div>

    <div class="slider-group" style="margin-top:14px;">
      <label><span>Độ sáng đèn nền LED (Matrix)</span><span id="ledBriVal">180</span></label>
      <input type="range" id="ledBriSlider" min="10" max="255" value="180">
    </div>

    <div class="grid-modes">
      <button class="mode-btn" onclick="setMode(6)">🏎️ Xe Đua / Custom</button>
      <button class="mode-btn" onclick="setMode(3)">🌈 Sóng Cầu Vồng</button>
      <button class="mode-btn" onclick="setMode(4)">🌊 Dòng Chảy Đuổi</button>
      <button class="mode-btn" onclick="setMode(2)">🫁 Nhịp Thở</button>
      <button class="mode-btn" onclick="setMode(1)">🎨 Đơn Sắc Xanh</button>
      <button class="mode-btn" onclick="setMode(0)">🌑 Tắt Hiển Thị</button>
    </div>
  </div>

  <!-- Wi-Fi Settings -->
  <div class="card">
    <h2>📶 Cấu Hình Wi-Fi Thiết Bị</h2>
    <input type="text" id="wifiSsid" placeholder="Tên Wi-Fi (SSID)">
    <input type="password" id="wifiPass" placeholder="Mật khẩu Wi-Fi">
    <button class="btn btn-outline" onclick="saveWifi()">Lưu & Kết Nối</button>
  </div>
</div>

<script>
const canvas = document.getElementById('previewCanvas');
const ctx = canvas.getContext('2d');
let loadedImg = null;

// Initial car canvas rendering
ctx.fillStyle = '#000';
ctx.fillRect(0,0,64,32);
ctx.fillStyle = '#00d2ff';
ctx.font = '7px sans-serif';
ctx.fillText('64x32 PREVIEW', 4, 18);

document.getElementById('imageInput').addEventListener('change', function(e) {
  const file = e.target.files[0];
  if (!file) return;
  document.getElementById('fileBtnLabel').innerText = '📁 ' + file.name;
  const reader = new FileReader();
  reader.onload = function(evt) {
    loadedImg = new Image();
    loadedImg.onload = renderPreview;
    loadedImg.src = evt.target.result;
  };
  reader.readAsDataURL(file);
});

document.getElementById('imgBriSlider').addEventListener('input', function(e) {
  document.getElementById('imgBriVal').innerText = e.target.value + '%';
  renderPreview();
});
document.getElementById('imgConSlider').addEventListener('input', function(e) {
  document.getElementById('imgConVal').innerText = e.target.value + '%';
  renderPreview();
});

function renderPreview() {
  if (!loadedImg) return;
  const bri = document.getElementById('imgBriSlider').value / 100;
  const con = document.getElementById('imgConSlider').value / 100;

  // Clear
  ctx.clearRect(0, 0, 64, 32);
  ctx.drawImage(loadedImg, 0, 0, 64, 32);

  const imgData = ctx.getImageData(0, 0, 64, 32);
  const d = imgData.data;
  for (let i = 0; i < d.length; i += 4) {
    // Contrast & Brightness
    for (let c = 0; c < 3; c++) {
      let v = d[i + c];
      v = ((v - 128) * con + 128) * bri;
      d[i + c] = Math.max(0, Math.min(255, v));
    }
  }
  ctx.putImageData(imgData, 0, 0);
}

document.getElementById('uploadBtn').addEventListener('click', async function() {
  const statusDiv = document.getElementById('uploadStatus');
  statusDiv.style.display = 'block';
  statusDiv.className = 'status-toast';
  statusDiv.innerText = 'Đang chuyển đổi và nạp lên ESP32...';

  try {
    const imgData = ctx.getImageData(0, 0, 64, 32).data;
    const bytes = new Uint8Array(64 * 32 * 2);
    let idx = 0;

    for (let i = 0; i < imgData.length; i += 4) {
      let r = imgData[i];
      let g = imgData[i + 1];
      let b = imgData[i + 2];

      let r5 = (r >> 3) & 0x1F;
      let g6 = (g >> 2) & 0x3F;
      let b5 = (b >> 3) & 0x1F;
      let col565 = (r5 << 11) | (g6 << 5) | b5;

      // Little-endian order for ESP32 uint16_t array
      bytes[idx++] = col565 & 0xFF;
      bytes[idx++] = (col565 >> 8) & 0xFF;
    }

    let binary = '';
    for (let i = 0; i < bytes.length; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    const b64 = btoa(binary);

    const res = await fetch('/api/upload', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ image: b64, width: 64, height: 32 })
    });

    if (res.ok) {
      statusDiv.className = 'status-toast status-ok';
      statusDiv.innerText = '✅ Đã chiếu ảnh lên bảng LED thành công!';
    } else {
      throw new Error('Lỗi upload');
    }
  } catch (err) {
    statusDiv.className = 'status-toast status-err';
    statusDiv.innerText = '❌ Nạp thất bại: ' + err.message;
  }
});

let powerState = true;
document.getElementById('pwrBtn').addEventListener('click', function() {
  powerState = !powerState;
  fetch('/api/command', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ power: powerState })
  });
});

document.getElementById('ledBriSlider').addEventListener('input', function(e) {
  document.getElementById('ledBriVal').innerText = e.target.value;
  fetch('/api/command', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ brightness: parseInt(e.target.value) })
  });
});

function setMode(modeId) {
  fetch('/api/command', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ mode: modeId })
  });
}

function saveWifi() {
  const ssid = document.getElementById('wifiSsid').value;
  const pass = document.getElementById('wifiPass').value;
  if (!ssid) return alert('Vui lòng nhập SSID');
  fetch('/api/wifi', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ ssid: ssid, password: pass })
  });
  alert('Đã gửi thông tin Wi-Fi. ESP32 đang kết nối lại...');
}
</script>
</body>
</html>
)rawliteral";

WifiManager& WifiManager::getInstance() {
    static WifiManager instance;
    return instance;
}

WifiManager::WifiManager()
    : lastReconnectAttempt(0), connected(false),
      server(80), webServerStarted(false) {}

void WifiManager::loadFromNVS() {
    Preferences prefs;
    prefs.begin(PREF_WIFI_NS, true);
    currentSsid = prefs.getString("ssid", WIFI_SSID_DEFAULT);
    currentPass = prefs.getString("pass", WIFI_PASS_DEFAULT);
    prefs.end();
}

void WifiManager::saveToNVS(const String& ssid, const String& password) {
    Preferences prefs;
    prefs.begin(PREF_WIFI_NS, false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    prefs.end();
    LOG_I(TAG, "Wi-Fi credentials saved to NVS (SSID: %s)", ssid.c_str());
}

void WifiManager::connectNewWifi(const String& ssid, const String& password) {
    currentSsid = ssid;
    currentPass = password;
    saveToNVS(ssid, password);

    LOG_I(TAG, "Initiating connection to new Wi-Fi: %s...", ssid.c_str());
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(currentSsid.c_str(), currentPass.c_str());
    lastReconnectAttempt = millis();
}

void WifiManager::init() {
    loadFromNVS();
    WiFi.mode(WIFI_STA);
    WiFi.begin(currentSsid.c_str(), currentPass.c_str());
    LOG_I(TAG, "Connecting to Wi-Fi SSID: %s...", currentSsid.c_str());
}

void WifiManager::startWebServer() {
    if (webServerStarted) return;

    server.on("/", HTTP_GET, [this]() {
        handleRoot();
    });

    server.on("/api/upload", HTTP_POST, [this]() {
        handleApiUpload();
    });

    server.on("/api/command", HTTP_POST, [this]() {
        handleApiCommand();
    });

    server.on("/api/wifi", HTTP_POST, [this]() {
        handleApiWifi();
    });

    server.on("/api/status", HTTP_GET, [this]() {
        handleApiStatus();
    });

    server.begin();
    webServerStarted = true;
    LOG_I(TAG, "HTTP Web Server started on port 80. Open http://%s in phone browser!", WiFi.localIP().toString().c_str());
}

void WifiManager::handleRoot() {
    server.send_P(200, "text/html; charset=UTF-8", INDEX_HTML);
}

void WifiManager::handleApiUpload() {
    if (OtaManager::getInstance().isBusy()) { server.send(409,"application/json","{\"error\":\"ota_busy\"}"); return; }
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"missing_body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err || !doc["image"].is<const char*>()) {
        server.send(400, "application/json", "{\"error\":\"invalid_json\"}");
        return;
    }

    const char* b64 = doc["image"].as<const char*>();
    uint8_t buffer[4096];
    size_t outLen = 0;
    int ret = mbedtls_base64_decode(buffer, sizeof(buffer), &outLen, (const unsigned char*)b64, strlen(b64));

    if (ret != 0 || outLen != 4096) {
        LOG_E(TAG, "Base64 decode failed or size != 4096 (got %d bytes)", (int)outLen);
        server.send(400, "application/json", "{\"error\":\"decode_failed\"}");
        return;
    }

    ProjectionHeader header;
    header.magic = PROJECTION_MAGIC;
    header.version = 1;
    header.frameCount = 1;
    header.width = 64;
    header.height = 32;
    header.frameDelayMs = 200;
    header.colorFormat = 0;

    bool saved = StorageManager::getInstance().savePattern(DEFAULT_PATTERN_PATH, header, buffer, outLen);
    if (saved) {
        EffectManager::getInstance().getCustomEffect().setFrames(header, buffer, outLen);
        StateManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
        EffectManager::getInstance().setMode(EffectMode::CUSTOM_PRESET);
        LOG_I(TAG, "Web Upload: Image projected onto LED Matrix successfully!");
        server.send(200, "application/json", "{\"status\":\"ok\",\"mode\":6}");
    } else {
        server.send(500, "application/json", "{\"error\":\"flash_write_fail\"}");
    }
}

void WifiManager::handleApiCommand() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"missing_body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "application/json", "{\"error\":\"invalid_json\"}");
        return;
    }

    StateManager& state = StateManager::getInstance();

    if (doc["power"].is<bool>()) {
        state.setPower(doc["power"].as<bool>());
    }
    if (doc["brightness"].is<uint8_t>() || doc["brightness"].is<int>()) {
        state.setBrightness(doc["brightness"].as<uint8_t>());
    }
    if (doc["speed"].is<uint8_t>() || doc["speed"].is<int>()) {
        state.setSpeed(doc["speed"].as<uint8_t>());
    }
    if (doc["mode"].is<uint8_t>() || doc["mode"].is<int>()) {
        uint8_t m = doc["mode"].as<uint8_t>();
        state.setMode(static_cast<EffectMode>(m));
        EffectManager::getInstance().setMode(static_cast<EffectMode>(m));
    }
    if (doc["color"].is<JsonObject>()) {
        uint8_t r = doc["color"]["r"] | 255;
        uint8_t g = doc["color"]["g"] | 255;
        uint8_t b = doc["color"]["b"] | 255;
        state.setPrimaryColor(r, g, b);
        state.setMode(EffectMode::STATIC_COLOR);
        EffectManager::getInstance().setMode(EffectMode::STATIC_COLOR);
    }

    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WifiManager::handleApiWifi() {
    if (OtaManager::getInstance().isBusy()) { server.send(409,"application/json","{\"error\":\"ota_busy\"}"); return; }
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"missing_body\"}");
        return;
    }

    JsonDocument doc;
    deserializeJson(doc, server.arg("plain"));
    const char* ssid = doc["ssid"] | "";
    const char* pass = doc["password"] | "";

    if (strlen(ssid) > 0) {
        server.send(200, "application/json", "{\"status\":\"reconnecting\"}");
        connectNewWifi(ssid, pass);
    } else {
        server.send(400, "application/json", "{\"error\":\"empty_ssid\"}");
    }
}

void WifiManager::handleApiStatus() {
    DeviceSettings& s = StateManager::getInstance().getSettings();
    String json = "{";
    json += "\"power\":" + String(s.powerOn ? "true" : "false") + ",";
    json += "\"brightness\":" + String(s.brightness) + ",";
    json += "\"speed\":" + String(s.speed) + ",";
    json += "\"mode\":" + String(static_cast<int>(s.currentMode)) + ",";
    json += "\"ip\":\"" + getIpAddress() + "\",";
    json += "\"emergency_stop\":" + String(StateManager::getInstance().isEmergencyStopped() ? "true" : "false") + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap());
    json += "}";
    server.send(200, "application/json", json);
}

void WifiManager::update() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!connected) {
            connected = true;
            LOG_I(TAG, "Wi-Fi Connected! IP: %s (SSID: %s)", WiFi.localIP().toString().c_str(), currentSsid.c_str());
            startWebServer();
            BleManager::getInstance().sendStatusNotification(
                "{\"event\":\"wifi_connected\",\"ip\":\"" + getIpAddress() + "\",\"ssid\":\"" + currentSsid + "\"}"
            );
        }
        if (webServerStarted) {
            server.handleClient();
        }
    } else {
        if (connected) {
            connected = false;
            LOG_W(TAG, "Wi-Fi Disconnected!");
            BleManager::getInstance().sendStatusNotification("{\"event\":\"wifi_disconnected\"}");
        }

        if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL_MS) {
            lastReconnectAttempt = millis();
            LOG_I(TAG, "Attempting Wi-Fi reconnection...");
            WiFi.reconnect();
        }
    }
}

bool WifiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getIpAddress() {
    return WiFi.localIP().toString();
}
