/*
 * http_config_server.cpp
 *
 *      Author: christophoberle
 *
 * this work is licenced under the Apache 2.0 licence
 *
 * protocol_examples_utils is licenced under the BSD 2-Clause "Simplified" Licence
 *
 */

#include "http_config_server.hpp"

#include "generic_nvsflash.hpp"

#include "protocol_examples_utils.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "esp_idf_version.h"

HttpConfigServer& HttpConfigServer::getInstance()
{
    static HttpConfigServer instance; // Guaranteed to be destroyed. Instantiated on first use.
    return instance;
}


bool HttpConfigServer::initialize( std::string tag,
                                   std::string ipAddress,
                                   std::string nvsNamespace
                                 ) {
    if (this->initializationDone) {
        ESP_LOGI(this->tag.c_str(), "initialize not executed, HttpConfigServer is already initialized!");
        return false;
    }
    else {
        ESP_LOGI(this->tag.c_str(), "initialize HttpConfigServer");
	    this->tag = tag;
	    this->ipAddress = ipAddress;
        this->nvsNamespace = nvsNamespace;

        this->initializationDone = true;

        return true;
    }
}

HttpConfigServer::~HttpConfigServer() {
}

bool HttpConfigServer::isConfigured()
{
    bool allParametersConfigured = true;
    std::string parameterValue;
    esp_err_t ret;

    // if configuration is active (webserver is running) -> return false
    if (this->configurationActive) {
        //ESP_LOGI(this->tag.c_str(), "isConfigured: configurationActive: address: %x value: %s", &this->configurationActive, this->configurationActive ? "true" : "false");
        ESP_LOGI(this->tag.c_str(), "isConfigured: configurationActive == true -> return false");
        return false;
    }

    // if configuration is not active, check whether all parameters are configured
    // Iterate parameters
    for (const auto& [parameterName, description] : this->parameters) {
        //ESP_LOGI(this->tag.c_str(), "isConfigured: [%s] Description: \"%s\"", parameterName.c_str(), description.c_str());

        parameterValue = this->getStringParameterValue(parameterName, &ret);
        ESP_LOGI(this->tag.c_str(), "isConfigured: Parameter %s: getStringParameterValue ret: %u value: %s", parameterName.c_str(), ret, parameterValue.c_str());

        if (ret != ESP_OK) {
            allParametersConfigured = false;
        }
    }

    if (!allParametersConfigured) {
        ESP_LOGI(this->tag.c_str(), "isConfigured: not allParametersConfigured -> startConfiguration -> return false");
        // start HTTP configuration
        this->startConfiguration();
        return false;
    }
    else {
        ESP_LOGI(this->tag.c_str(), "isConfigured: allParametersConfigured -> return true");
        return true;
    }
}

esp_err_t HttpConfigServer::addStringParameter( std::string parameterName,
                                                std::string description )
{
    ESP_LOGI(this->tag.c_str(), "addStringParameter: %s (%s)", parameterName.c_str(), description.c_str());
    if ( parameterName.length() > 15) {
        ESP_LOGE(this->tag.c_str(), "addStringParameter: parameterName %s too long! size=%u (>15)", parameterName.c_str(), parameterName.length());
        return ESP_FAIL;
    }
    else {
        this->parameters[parameterName] = description;
        return ESP_OK;
    }
}

esp_err_t HttpConfigServer::setStringParameterValue( std::string parameterName,
                                                     std::string parameterValue )
{
    esp_err_t ret;

    GenericNvsFlash *nvsFlash = new GenericNvsFlash("nvsFlash", this->nvsNamespace, NVS_READWRITE);
    ret = nvsFlash->SetStr(parameterName, parameterValue);
    ESP_LOGI(this->tag.c_str(), "setStringParameterValue: %s = %s ret=%u", parameterName.c_str(), parameterValue.c_str(), ret);
   delete nvsFlash;


    return ret;
}

esp_err_t HttpConfigServer::eraseParameter( std::string parameterName )
{
    esp_err_t ret;

    ESP_LOGI(this->tag.c_str(), "eraseParameter: %s", parameterName.c_str());
    GenericNvsFlash *nvsFlash = new GenericNvsFlash("nvsFlash", this->nvsNamespace, NVS_READWRITE);
    ret = nvsFlash->EraseKey(parameterName);
    delete nvsFlash;

    return ret;
}

std::string HttpConfigServer::getStringParameterValue(std::string parameterName, esp_err_t *ret )
{
    ESP_LOGI(this->tag.c_str(), "getStringParameterValue: %s", parameterName.c_str());
    GenericNvsFlash *nvsFlash = new GenericNvsFlash("nvsFlash", this->nvsNamespace, NVS_READWRITE);
    std::string parameterValue = nvsFlash->GetStr(parameterName, ret);
    delete nvsFlash;

    return parameterValue;
}

void HttpConfigServer::startConfiguration() {
    ESP_LOGI(this->tag.c_str(), "startConfiguration");
    this->configurationActive = true;

    /* Start HTTP server */
    ESP_LOGI(this->tag.c_str(), "startConfiguration: configurationActive: address: %x value: %s", &this->configurationActive, this->configurationActive ? "true" : "false");
    this->server = this->startWebserver();
}

void HttpConfigServer::stopConfiguration() {
    ESP_LOGI(this->tag.c_str(), "stopConfiguration");
    ESP_LOGI(this->tag.c_str(), "stopConfiguration: configurationActive: address: %x value: %s", &this->configurationActive, this->configurationActive ? "true" : "false");
    this->configurationActive = false;
    ESP_LOGI(this->tag.c_str(), "stopConfiguration: configurationActive: address: %x value: %s", &this->configurationActive, this->configurationActive ? "true" : "false");
    /* Stop HTTP server */
    this->stopWebserver(this->server);
}

/* An HTTP GET handler for the config page */
extern "C" esp_err_t config_get_handler(httpd_req_t *req)
{
    // get ConfigServer instance
    HttpConfigServer* configServer = &HttpConfigServer::getInstance();
    return configServer->configGetHandler(req);
}

/* An HTTP GET handler */
esp_err_t HttpConfigServer::configGetHandler(httpd_req_t *req)
{
    ESP_LOGI(this->tag.c_str(), "configGetHandler");
    char* buf;
    size_t buf_len;
    bool queryBuffer = false;

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = (char *) malloc(buf_len);
        //ESP_RETURN_ON_FALSE(buf, ESP_ERR_NO_MEM, this->tag.c_str(), "buffer alloc failed"); ToDo!
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(this->tag.c_str(), "Found URL query => %s", buf);
            queryBuffer = true;
        }
    }

    esp_err_t ret;
    std::string resp_str;
    resp_str += "<head>";
    resp_str += "<style type=\"text/css\">\n";
    resp_str += "     /* ========== Base Styles ========== */\n";
    resp_str += "     body {\n";
    resp_str += "         font-family: Arial, sans-serif;\n";
    resp_str += "         margin: 10px;\n";
    resp_str += "         padding: 0;\n";
    resp_str += "         background-color: #f0f0f0;\n";
    resp_str += "     }\n";
    resp_str += "     h3 {\n";
    resp_str += "         width: 15%;\n";
    resp_str += "         float: left;\n";
    resp_str += "         vertical-align: top\n";
    resp_str += "         padding: 0;\n"; // 5px
    //resp_str += "         box-sizing: border-box;\n";
    //resp_str += "         border: 1px solid #ccc;\n";
    //resp_str += "         border-radius: 3px;\n";
    resp_str += "         font-size: 1em;\n";
    resp_str += "     }\n";
    resp_str += "     p {\n";
    //resp_str += "         width: 15%;\n";
    //resp_str += "         float: left;\n";
    //resp_str += "         vertical-align: top\n";
    resp_str += "         margin-top: 5px;\n";
    resp_str += "         padding: 5px;\n";
    //resp_str += "         box-sizing: border-box;\n";
    //resp_str += "         border: 1px solid #ccc;\n";
    //resp_str += "         border-radius: 3px;\n";
    resp_str += "         font-size: 1em;\n";
    resp_str += "     }\n";
      resp_str += " \n";
    resp_str += "     /* ========== Form Elements ========== */\n";
    resp_str += "     label {\n";
    resp_str += "         display: block;\n";
    resp_str += "         margin-bottom: 5px;\n";
    resp_str += "     }\n";
    resp_str += "     input {\n";
    resp_str += "         width: 85%;\n";
    resp_str += "         float: right;\n";
    resp_str += "         padding: 5px;\n";
    resp_str += "         margin-bottom: 5px;\n";
    resp_str += "         box-sizing: border-box;\n";
    resp_str += "         border: 1px solid #ccc;\n";
    resp_str += "         border-radius: 3px;\n";
    resp_str += "         font-size: 1em;\n";
    resp_str += "     }\n";
    resp_str += "     button {\n";
    resp_str += "         width: 100%;\n";
    resp_str += "         padding: 5px;\n";
    resp_str += "         box-sizing: border-box;\n";
    resp_str += "         border: 1px solid #ccc;\n";
    resp_str += "         border-radius: 3px;\n";
    resp_str += "         font-size: 1em;\n";
    resp_str += "     }\n";
    resp_str += "     button[type=\"submit\"] {\n";
    resp_str += "         background-color: #007bff;\n";
    resp_str += "         color: #fff;\n";
    resp_str += "         border: none;\n";
    resp_str += "         border-radius: 3px;\n";
    resp_str += "         padding: 10px;\n";
    resp_str += "         cursor: pointer;\n";
    resp_str += "     }\n";
    resp_str += "     button[type=\"submit\"]:hover {\n";
    resp_str += "         background-color: #0056b3;\n";
    resp_str += "     }\n";
    resp_str += "     button[type=\"submit\"]:disabled {\n";
    resp_str += "         background-color: #ccc;\n";
    resp_str += "         cursor: not-allowed;\n";
    resp_str += "     }\n";
    resp_str += "\n";
    resp_str += "  </style>\n";
    resp_str += "</head>";
    resp_str += "<body>";
    resp_str += "<h1>Config Page</h1>";

    resp_str += "<form id=\"config\">";
    resp_str += "<h2>Configuration of Parameters</h2>";
    //resp_str += "<br/>";

    bool allParametersConfigured = true;

    // Iterate parameters
    for (const auto& [parameterName, description] : this->parameters) {
        std::string parameter;
        bool thisParameterConfigured = false;

        std::string parameterValue = this->getStringParameterValue(parameterName, &ret);
        if (ret != 0) {
            parameterValue = std::string("");
        }
        else {
            thisParameterConfigured = true;
        }

        if (queryBuffer) {
            char param[200], dec_param[200] = {0};
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, parameterName.c_str(), param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(this->tag.c_str(), "Found URL query parameter => %s=%s", parameterName.c_str(), param);
                example_uri_decode(dec_param, param, strnlen(param, 200));
                ESP_LOGI(this->tag.c_str(), "Decoded query parameter => %s", dec_param);

                parameterValue = dec_param;

                if (parameterValue.length() > 0) {
                    this->setStringParameterValue(parameterName, parameterValue);
                    thisParameterConfigured = true;
                }
                else {
                    this->eraseParameter(parameterName);
                    thisParameterConfigured = false;
                }
            }
        }
        if (thisParameterConfigured) {
        }
        else {
            allParametersConfigured = false;
        }

        resp_str += "<hr><h3>" + parameterName + ":</h3>";
        resp_str += "<input type=\"text\" name=\"" + parameterName + "\" id=\"" + parameterName + "\" size=\"200\" maxlength=\"200\" value=\"" + parameterValue + "\">";
        resp_str += "<p>";

        std::string desc = description;
        while(desc.find(std::string("<"))!=std::string::npos) {
            desc.replace(desc.find(std::string("<")),std::string("<").length(),std::string("&lt;"));
        }
        while(desc.find(std::string(">"))!=std::string::npos) {
            desc.replace(desc.find(std::string(">")),std::string(">").length(),std::string("&gt;"));
        }

        resp_str += desc;
        resp_str += "</p>";
        //resp_str += "<br/>";
    }
    resp_str += "<button type=\"submit\">submit</button>";
    resp_str += "</form>";
    resp_str += "</body>";


    if (buf_len > 1) {
        free(buf);
    }

    httpd_resp_send(req, resp_str.c_str(), HTTPD_RESP_USE_STRLEN);

    if (allParametersConfigured) {
        ESP_LOGI(this->tag.c_str(), "configGetHandler: allParametersConfigured -> stopConfiguration");
        this->stopConfiguration();
    }

    return ESP_OK; // was: ESP_FAIL to close the connection
}

const httpd_uri_t uriConfig = {
    .uri       = "/config",
    .method    = HTTP_GET,
    .handler   = config_get_handler,
    .user_ctx  = NULL
};

httpd_handle_t HttpConfigServer::startWebserver()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(this->tag.c_str(), "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&this->server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(this->tag.c_str(), "Registering URI handlers");
        httpd_register_uri_handler(this->server, &uriConfig);
        return server;
    }

    ESP_LOGI(this->tag.c_str(), "Error starting HTTP server!");
    return NULL;
}

esp_err_t HttpConfigServer::stopWebserver(httpd_handle_t server)
{
    esp_err_t ret;
    ESP_LOGI(this->tag.c_str(), "stopWebserver");
    // Stop the httpd server
    ret = httpd_stop(server);
    if (ret == ESP_OK) {
        server = nullptr;
    }
    return ret;
}

