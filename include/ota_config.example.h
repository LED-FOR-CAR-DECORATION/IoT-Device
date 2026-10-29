#pragma once
// Copy to ota_config.h before building bootstrap and subsequent OTA releases.
#define OTA_API_BASE "https://api.example.com"
// Public PEM root CA validating this host. No trailing slash on OTA_API_BASE.
#define OTA_ROOT_CA R"PEM(-----BEGIN CERTIFICATE-----
REPLACE_WITH_YOUR_HOST_ROOT_CA
-----END CERTIFICATE-----
)PEM"
