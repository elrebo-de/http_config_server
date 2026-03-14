/*
 * http_config_server.hpp
 *
 *      Author: christophoberle
 *
 * this work is licenced under the Apache 2.0 licence
 */

#ifndef HTTP_CONFIG_SERVER_HPP_
#define HTTP_CONFIG_SERVER_HPP_

#include <string>
#include <map>

#include <esp_http_server.h>

#include "esp_log.h"

#include "generic_nvsflash.hpp"

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

#endif /* HTTP_CONFIG_SERVER_HPP_ */
