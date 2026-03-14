# HTTP Config Server component

ESP-IDF configuration component.
Configuration parameters are stored in nvs_flash.
To enter the parameters an HTTP server is started.
It runs on any ESP32 processor and is built using the ESP-IDF build system in version 5.5+.

The component is implemented as C++ class `HttpConfigServer`.

## Connecting the component

The constructor of class `HttpConfigServer` has two parameters:

| Parameter | Type of Parameter | Usage                                              |
|:----------|:------------------|:---------------------------------------------------|
| tag       | std::string       | the tag to be used in the ESP log                  |
| ipAddress | std::string       | the IP address on which the HTTP Server is started |
| config    | std::string       | the nvs namespace                                  |

# Usage
To use the HttpConfigServer component you have to include "http_config_server.hpp".

Configuration parameters are added with method `addString`. Currently only parameters of type std::string can be added.

After defining the configuration parameters the class must be initialized with method `initialize`.

The method first checks, whether the paramaters are already defined in nvs_flash. If they are already defined there, the method returns.

If they are not yet stored in nvs_flash, an HTTP server is started, which enables the user to set the configuration parameters in nvs_flash.
When the configuration is finished, the method returns.

The configuration parameters can be retrieved with method `getString`.

Method EnableTimerWakeup has two parameters:

| Parameter     | Type of Parameter | Usage                                                                            |
|:--------------|:------------------|:---------------------------------------------------------------------------------|
| sleepTime     | unsigned long     | the sleep time                                                                   |
| sleepTimeUnit | std::string       | the unit in which the sleep time is given {"min", "sec", "milliSec", "microSec"} |

Method EnableGpioWakeup has two parameters:

| Parameter | Type of Parameter | Usage                           |
|:----------|:------------------|:--------------------------------|
| gpio      | gpio_num_t        | the GPIO number                 |
| level     | int               | the wakeup trigger level {1, 0} |

Example code:

```
#include "wifi_manager.hpp"

#include "http_config_server.hpp"

// InfluxDB server url. Don't use localhost, always server name or ip address.
// E.g. http://192.168.1.48:8086 (In InfluxDB 2 UI -> Load Data -> Client Libraries),
std::string influxdbUrl = "";
// InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
std::string influxdbToken = "";
// InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
std::string influxdbOrg = "";
// InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
std::string influxdbBucket = "";

extern "C" void app_main(void)
{
    Wifi wifi( std::string("WifiManager"), // tag for ESP_LOGx
               std::string("ESP32"),       // ssid_prefix for configuration access point
               std::string("de-DE")        // language for configuration access point
             );

    // construct HttpConfigServer instance
    HttpConfigServer configServer( std::string("HttpServerConfig"), // tag for ESP_LOGx
                                   wifi.GetIpAddress(), // IP address
                                   std::string("config")
                                 );
    // add config parameter "influxdbUrl"                                 
    configServer.addString( "influxdbUrl", 
                            "",
                            "InfluxDB server url. Don't use localhost, always server name or ip address.\n" +
                            "E.g. http://192.168.1.48:8086 (In InfluxDB 2 UI -> Load Data -> Client Libraries)" 
                          );                                                          
    // add config parameter "influxdbToken"                                 
    configServer.addString( "influxdbToken", 
                            "",
                            "InfluxDB 2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)" 
                          );                                                          
    // add config parameter "influxdbOrg"                                 
    configServer.addString( "influxdbOrg", 
                            "",
                            "InfluxDB 2 organization id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )"
                          );                                                          
    // add config parameter "influxdbBucket"                                 
    configServer.addString( "influxdbBucket", 
                            "",
                            "InfluxDB 2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)"
                          );

    // initialize configServer
    // connect to nvs_flash
    // if config parameters are already set in nvs_flash -> return
    // else start http server to set config parameter values
    // return after config parameters are set in nvs_flash
    configServer.initialize();

    // get config paramter values
    influxdbUrl = configServer.getString("influxdbUrl");
    influxdbToken = configServer.getString("influxdbToken");
    influxdbOrg = configServer.getString("influxdbOrg");
    influxdbBucket = configServer.getString("influxdbBucket");
    
    ...
    
}
```

## API
The API of the component is located in the include directory ```include/http_config_server.hpp``` and defines the
C++ class `HttpConfigServer`

```C++
```

# License
This component is provided under the Apache 2.0 license.

It uses protocol_examples_utils from GitHub repository espressif/esp-idf at 
examples/common_components/protocol_examples_common/include/protocol_examples_utils.h and 
examples/common_components/protocol_examples_common/protocol_examples_utils.c
which is provided under the BSD 2-Clause "Simplified" License.