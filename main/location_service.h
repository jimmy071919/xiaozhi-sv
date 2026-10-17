#ifndef LOCATION_SERVICE_H
#define LOCATION_SERVICE_H

#include <cJSON.h>

#include <mutex>
#include <string>

// User location for the agent: a city-level estimate from the public IP.
class LocationService {
public:
    static LocationService& GetInstance() {
        static LocationService instance;
        return instance;
    }

    LocationService(const LocationService&) = delete;
    LocationService& operator=(const LocationService&) = delete;

    // Blocking HTTP lookup; call from a background task, never the main loop.
    void DetectByIp();

    // Caller owns the returned object
    cJSON* GetLocationJson();

private:
    LocationService();

    std::mutex mutex_;
    std::string ip_city_;
    std::string ip_region_;
    std::string ip_country_;
    std::string ip_coordinates_;
    std::string ip_timezone_;
};

#endif  // LOCATION_SERVICE_H
