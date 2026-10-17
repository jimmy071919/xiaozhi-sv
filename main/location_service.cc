#include "location_service.h"

#include <esp_log.h>

#include "board.h"
#include "cjson_utils.h"
#include "markdown_text.h"
#include "settings.h"

#define TAG "Location"

namespace {

constexpr const char* kIpLookupUrl = "https://ipinfo.io/json";
constexpr size_t kMaxFieldBytes = 64;

std::string GetStringField(const cJSON* root, const char* name) {
    auto item = cJSON_GetObjectItem(root, name);
    if (!cJSON_IsString(item)) {
        return "";
    }
    return TruncateUtf8(item->valuestring, kMaxFieldBytes);
}

}  // namespace

LocationService::LocationService() {
    // Location is always detected by IP now; drop a location saved by the
    // removed self.location.set tool
    Settings settings("location", true);
    settings.EraseKey("manual");
}

void LocationService::DetectByIp() {
    auto network = Board::GetInstance().GetNetwork();
    auto http = network->CreateHttp(0);
    http->SetTimeout(5000);
    http->SetHeader("Accept", "application/json");
    if (auto opened = http->Open("GET", kIpLookupUrl); !opened) {
        ESP_LOGW(TAG, "IP lookup failed: %s", opened.error().ToString().c_str());
        return;
    }
    auto status_code = http->GetStatusCode();
    if (!status_code || *status_code != 200) {
        ESP_LOGW(TAG, "IP lookup returned status %d", status_code ? *status_code : -1);
        http->Close();
        return;
    }
    auto body = http->ReadAll();
    http->Close();

    CJsonUniquePtr root(cJSON_ParseWithLength(body.data(), body.size()));
    if (!cJSON_IsObject(root.get())) {
        ESP_LOGW(TAG, "IP lookup returned invalid JSON");
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    ip_city_ = GetStringField(root.get(), "city");
    ip_region_ = GetStringField(root.get(), "region");
    ip_country_ = GetStringField(root.get(), "country");
    ip_coordinates_ = GetStringField(root.get(), "loc");
    ip_timezone_ = GetStringField(root.get(), "timezone");
    ESP_LOGI(TAG, "IP location: %s, %s, %s", ip_city_.c_str(), ip_region_.c_str(),
             ip_country_.c_str());
}

cJSON* LocationService::GetLocationJson() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto json = cJSON_CreateObject();
    if (!ip_city_.empty() || !ip_country_.empty()) {
        cJSON_AddStringToObject(json, "source", "ip");
        cJSON_AddStringToObject(json, "accuracy", "city-level estimate, may be wrong on 4G or VPN");
        cJSON_AddStringToObject(json, "city", ip_city_.c_str());
        cJSON_AddStringToObject(json, "region", ip_region_.c_str());
        cJSON_AddStringToObject(json, "country", ip_country_.c_str());
        if (!ip_coordinates_.empty()) {
            cJSON_AddStringToObject(json, "coordinates", ip_coordinates_.c_str());
        }
    } else {
        cJSON_AddStringToObject(json, "source", "unknown");
    }
    if (!ip_timezone_.empty()) {
        cJSON_AddStringToObject(json, "timezone", ip_timezone_.c_str());
    }
    return json;
}
