/*
 * Example program to use http config server with elrebo-de/deep_sleep
 */

#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "http_config_server.hpp"
#include "wifi_manager.hpp"

#include "esp_log.h"
#include "sdkconfig.h"

static const char *tag = "http config server test";

extern "C" void app_main(void)
{
    // short delay to reconnect logging
    vTaskDelay(pdMS_TO_TICKS(500)); // delay 0.5 seconds

    ESP_LOGI(tag, "Example Program");

    Wifi wifi( std::string("WifiManager"), // tag for ESP_LOGx
               std::string("ESP32"),       // ssid_prefix for configuration access point
               std::string("de-DE")        // language for configuration access point
             );

    // get ConfigServer instance
    HttpConfigServer* configServer = &HttpConfigServer::getInstance();

    configServer->initialize( std::string("HttpServerConfig"), // tag for ESP_LOGx
                              wifi.GetIpAddress(), // IP address
                              std::string("config") // nvs namespace
                            );

    // add config parameter "influxdbUrl"
    configServer->addStringParameter( "influxdbUrl",
                                     "InfluxDB server url. Don't use localhost, always server name or ip address. E.g. http://192.168.1.48:8086 (In InfluxDB 2 UI -> Load Data -> Client Libraries)"
                                   );
    // add config parameter "influxdbToken"
    configServer->addStringParameter( "influxdbToken",
                                     "InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)"
                                   );
    // add config parameter "influxdbOrg"
    configServer->addStringParameter( "influxdbOrg",
                                     "InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )"
                                   );
    // add config parameter "influxdbBucket"
    configServer->addStringParameter( "influxdbBucket",
                                     "InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)"
                                   );

    // isConfigured()
    // connect to nvs_flash
    // if config parameters are already set in nvs_flash -> return true
    // else start http server to set config parameter values and return false
    while (!configServer->isConfigured()) {
        ESP_LOGI(tag, "HttpConfigServer is not configured");
        vTaskDelay(pdMS_TO_TICKS(10000)); // delay 10 seconds
    }

    // get config parameter values
    esp_err_t ret;
    std::string influxdbUrl = configServer->getStringParameterValue("influxdbUrl", &ret);
    std::string influxdbToken = configServer->getStringParameterValue("influxdbToken", &ret);
    std::string influxdbOrg = configServer->getStringParameterValue("influxdbOrg", &ret);
    std::string influxdbBucket = configServer->getStringParameterValue("influxdbBucket", &ret);

    ESP_LOGI(tag, "influxdbUrl: %s", influxdbUrl.c_str());
    ESP_LOGI(tag, "influxdbToken: %s", influxdbToken.c_str());
    ESP_LOGI(tag, "influxdbOrg: %s", influxdbOrg.c_str());
    ESP_LOGI(tag, "influxdbBucket: %s", influxdbBucket.c_str());

    vTaskDelay(pdMS_TO_TICKS(60000)); // delay 1 minute
}
