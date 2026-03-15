# HTTP Config Server component

ESP-IDF configuration component.
Configuration parameters are stored in nvs_flash.
To enter the parameters an HTTP server is started.
It runs on any ESP32 processor and is built using the ESP-IDF build system in version 5.5+.

The component is implemented as C++ class `HttpConfigServer`.

## Connecting the component

The constructor of class `HttpConfigServer` has three parameters:

| Parameter | Type of Parameter | Usage                                              |
|:----------|:------------------|:---------------------------------------------------|
| tag       | std::string       | the tag to be used in the ESP log                  |
| ipAddress | std::string       | the IP address on which the HTTP Server is started |
| config    | std::string       | the name of the nvs namespace                      |

# Usage
To use the HttpConfigServer component you have to include "http_config_server.hpp".

Class HttpConfigServer is a Singleton. You get the instance with the static method `getInstance`.

First the class must be initialized with method `initialize`.

Then Configuration parameters are added with method `addStringParameter`. Currently only parameters of type std::string can be added.

Finally method `isInitialized` is used.

The method first checks, whether the parameters are already defined in nvs_flash. If they are already defined there, the method returns.

If they are not yet stored in nvs_flash, an HTTP server is started, which enables the user to set the configuration parameters in nvs_flash.

When the configuration is finished, the method returns.

Now the configuration parameters can be retrieved with method `getString`.


Example code:

```
#include "http_config_server.hpp"
#include "wifi_manager.hpp"

#include "esp_log.h"
#include "sdkconfig.h"

static const char *tag = "**** Example Program ****";

extern "C" void app_main(void)
{
    // short delay to reconnect logging
    vTaskDelay(pdMS_TO_TICKS(500)); // delay 0.5 seconds

    ESP_LOGI(tag, "Example Program Start");

    Wifi wifi( std::string("WifiManager"), // tag for ESP_LOGx
               std::string("ESP32"),       // ssid_prefix for configuration access point
               std::string("de-DE")        // language for configuration access point
             );

    // get ConfigServer instance
    HttpConfigServer* configServer = &HttpConfigServer::getInstance();

    configServer->initialize( std::string("**** HttpServerConfig ****"), // tag for ESP_LOGx
                              wifi.GetIpAddress(), // IP address
                              std::string("config") // nvs namespace
                            );

    // add config parameter "influxdbUrl"
    configServer->addStringParameter( "01_Url",
                                     "InfluxDB server url. Don't use localhost, always server name or ip address. E.g. http://192.168.1.48:8086 (In InfluxDB 2 UI -> Load Data -> Client Libraries)"
                                   );
    // add config parameter "influxdbToken"
    configServer->addStringParameter( "02_Token",
                                     "InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)"
                                   );
    // add config parameter "influxdbOrg"
    configServer->addStringParameter( "03_Org",
                                     "InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )"
                                   );
    // add config parameter "influxdbBucket"
    configServer->addStringParameter( "04_Bucket",
                                     "InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)"
                                   );

    // isConfigured()
    // connect to nvs_flash
    // if config parameters are already set in nvs_flash -> return true
    // else start http server to set config parameter values and return false
    while (!configServer->isConfigured()) {
        ESP_LOGI(tag, "HttpConfigServer is not yet configured");
        vTaskDelay(pdMS_TO_TICKS(10000)); // delay 10 seconds
    }

    // get config parameter values
    ESP_LOGI(tag, "get config parameter values from HttpConfigServer");
    esp_err_t ret;
    std::string influxdbUrl = configServer->getStringParameterValue("01_Url", &ret);
    std::string influxdbToken = configServer->getStringParameterValue("02_Token", &ret);
    std::string influxdbOrg = configServer->getStringParameterValue("03_Org", &ret);
    std::string influxdbBucket = configServer->getStringParameterValue("04_Bucket", &ret);

    ESP_LOGI(tag, "influxdbUrl: %s", influxdbUrl.c_str());
    ESP_LOGI(tag, "influxdbToken: %s", influxdbToken.c_str());
    ESP_LOGI(tag, "influxdbOrg: %s", influxdbOrg.c_str());
    ESP_LOGI(tag, "influxdbBucket: %s", influxdbBucket.c_str());

    vTaskDelay(pdMS_TO_TICKS(60000)); // delay 1 minute
}
```

## API
The API of the component is located in the include directory ```include/http_config_server.hpp``` and defines the
C++ class `HttpConfigServer`

```C++
/* class HttpConfigServer
   ESP-IDF configuration component.
   Configuration parameters are stored in nvs_flash.
   To enter the parameters an HTTP server is started.
   It runs on any ESP32 processor and is built using the ESP-IDF build system in version 5.5+.

   HttpConfigServer is a Singleton

   Tested with
   * ESP32C3
*/
class HttpConfigServer {
public:
    static HttpConfigServer& getInstance();

	bool initialize( std::string tag,
	                 std::string ipAddress,
	                 std::string nvsNamespace
	               );
	bool isConfigured();

    esp_err_t addStringParameter( std::string parameterName,
                                  std::string description
                                );

    esp_err_t setStringParameterValue( std::string parameterName,
                                       std::string parameterValue
                                );

    esp_err_t eraseParameter( std::string parameterName);

    std::string getStringParameterValue( std::string parameterName, esp_err_t *ret );

    esp_err_t configGetHandler(httpd_req_t *req);

private:
    HttpConfigServer() {} // Constructor
    virtual ~HttpConfigServer();

    void startConfiguration();
    void stopConfiguration();
    httpd_handle_t startWebserver();
    esp_err_t stopWebserver(httpd_handle_t server);

    std::string tag = "HttpConfigServer";
    std::string ipAddress;
    std::string nvsNamespace = "config";

    // Map of config parameters<parameterName, description>
    std::map<std::string, std::string> parameters{};

    bool initializationDone = false;
    bool configurationActive = false;
    httpd_handle_t server = NULL;

    public:
        HttpConfigServer(HttpConfigServer const&) = delete;
        void operator=(HttpConfigServer const&) = delete;
};
```

# License
This component is provided under the Apache 2.0 license.

It uses protocol_examples_utils from GitHub repository espressif/esp-idf at 
examples/common_components/protocol_examples_common/include/protocol_examples_utils.h and 
examples/common_components/protocol_examples_common/protocol_examples_utils.c
which is provided under the BSD 2-Clause "Simplified" License.