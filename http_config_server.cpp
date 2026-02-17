/*
 * http_config_server.cpp
 *
 *      Author: christophoberle
 *
 * this work is licenced under the Apache 2.0 licence
 */

#include "http_config_server.hpp"

#include "generic_nvsflash.hpp"

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
	    this->tag = tag;
	    this->ipAddress = ipAddress;
        this->nvsNamespace = nvsNamespace;

        this->nvsFlash = new GenericNvsFlash("nvsFlash", nvsNamespace, NVS_READWRITE);

        this->initializationDone = true;

        return true;
    }
}

HttpConfigServer::~HttpConfigServer() {
    if (nvsFlash != nullptr) {
        delete this->nvsFlash;
    }
    this->nvsFlash = nullptr;
}

bool HttpConfigServer::isConfigured()
{
    bool rc;
    bool allParametersConfigured = true;
    std::string parameterValue;
    esp_err_t ret;

    // Iterate parameters
    for (const auto& [parameterName, description] : this->parameters) {
        ESP_LOGI(this->tag.c_str(), "[%s] Description: \"%s\"", parameterName.c_str(), description.c_str());

        parameterValue = this->getStringParameterValue(parameterName, &ret);
        ESP_LOGI(this->tag.c_str(), "Parameter %s: getStringParameterValue ret: %u", parameterName.c_str(), ret);

        if (ret != ESP_OK) {
            allParametersConfigured = false;
        }
    }

    if (allParametersConfigured && this->configurationActive) {
        this->stopConfiguration();
    }

    if (this->configurationActive) {
        return false;
    }

    if (nvsFlash != nullptr) {
        delete this->nvsFlash;
    }
    this->nvsFlash = nullptr;

    if (!allParametersConfigured) {
        // start HTTP configuration
        this->startConfiguration();
        rc = false;
    }
    else {
        rc = true;
    }
    return rc;
}

esp_err_t HttpConfigServer::addStringParameter( std::string parameterName,
                                                std::string description )
{
    this->parameters[parameterName] = description;

    return ESP_OK; // ToDo !
}

esp_err_t HttpConfigServer::setStringParameterValue( std::string parameterName,
                                                     std::string parameterValue )
{
    if (this->nvsFlash == nullptr) {
        this->nvsFlash = new GenericNvsFlash("nvsFlash", nvsNamespace, NVS_READWRITE);
    }
    return nvsFlash->SetStr(parameterName, parameterValue);
}

esp_err_t HttpConfigServer::eraseParameter( std::string parameterName )
{
    if (this->nvsFlash == nullptr) {
        this->nvsFlash = new GenericNvsFlash("nvsFlash", nvsNamespace, NVS_READWRITE);
    }
    return nvsFlash->EraseKey(parameterName);
}

std::string HttpConfigServer::getStringParameterValue(std::string parameterName, esp_err_t *ret )
{
    *ret = ESP_OK;
    if (this->nvsFlash == nullptr) {
        this->nvsFlash = new GenericNvsFlash("nvsFlash", nvsNamespace, NVS_READWRITE);
    }
    return nvsFlash->GetStr(parameterName, ret);
}

void HttpConfigServer::startConfiguration() {
    this->configurationActive = true;

    /* Start HTTP server */
    this->server = this->startWebserver();
}

void HttpConfigServer::stopConfiguration() {
    /* Stop HTTP server */
    this->stopWebserver(this->server);

    this->configurationActive = false;
}

/* Type of Unescape algorithms to be used */
#define NGX_UNESCAPE_URI          (1)
#define NGX_UNESCAPE_REDIRECT     (2)


void ngx_unescape_uri(u_char **dst, u_char **src, size_t size, unsigned int type)
{
    u_char  *d, *s, ch, c, decoded;
    enum {
        sw_usual = 0,
        sw_quoted,
        sw_quoted_second
    } state;

    d = *dst;
    s = *src;

    state = sw_usual;
    decoded = 0;

    while (size--) {

        ch = *s++;

        switch (state) {
        case sw_usual:
            if (ch == '?'
                    && (type & (NGX_UNESCAPE_URI | NGX_UNESCAPE_REDIRECT))) {
                *d++ = ch;
                goto done;
            }

            if (ch == '%') {
                state = sw_quoted;
                break;
            }

            *d++ = ch;
            break;

        case sw_quoted:

            if (ch >= '0' && ch <= '9') {
                decoded = (u_char) (ch - '0');
                state = sw_quoted_second;
                break;
            }

            c = (u_char) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                decoded = (u_char) (c - 'a' + 10);
                state = sw_quoted_second;
                break;
            }

            /* the invalid quoted character */

            state = sw_usual;

            *d++ = ch;

            break;

        case sw_quoted_second:

            state = sw_usual;

            if (ch >= '0' && ch <= '9') {
                ch = (u_char) ((decoded << 4) + (ch - '0'));

                if (type & NGX_UNESCAPE_REDIRECT) {
                    if (ch > '%' && ch < 0x7f) {
                        *d++ = ch;
                        break;
                    }

                    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);

                    break;
                }

                *d++ = ch;

                break;
            }

            c = (u_char) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                ch = (u_char) ((decoded << 4) + (c - 'a') + 10);

                if (type & NGX_UNESCAPE_URI) {
                    if (ch == '?') {
                        *d++ = ch;
                        goto done;
                    }

                    *d++ = ch;
                    break;
                }

                if (type & NGX_UNESCAPE_REDIRECT) {
                    if (ch == '?') {
                        *d++ = ch;
                        goto done;
                    }

                    if (ch > '%' && ch < 0x7f) {
                        *d++ = ch;
                        break;
                    }

                    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);
                    break;
                }

                *d++ = ch;

                break;
            }

            /* the invalid quoted character */

            break;
        }
    }

done:

    *dst = d;
    *src = s;
}


void example_uri_decode(char *dest, const char *src, size_t len)
{
    if (!src || !dest) {
        return;
    }

    unsigned char *src_ptr = (unsigned char *)src;
    unsigned char *dst_ptr = (unsigned char *)dest;
    ngx_unescape_uri(&dst_ptr, &src_ptr, len, NGX_UNESCAPE_URI);
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
    char*  buf;
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
    resp_str += " \n";
    resp_str += "     /* ========== Form Elements ========== */\n";
    resp_str += "     label {\n";
    resp_str += "         display: block;\n";
    resp_str += "         margin-bottom: 5px;\n";
    resp_str += "     }\n";
    resp_str += "     input {\n";
    resp_str += "         width: 100%;\n";
    resp_str += "         padding: 5px;\n";
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
    resp_str += "<br/>";

    // Iterate parameters
    for (const auto& [parameterName, description] : this->parameters) {
        std::string parameter;

        std::string parameterValue = this->getStringParameterValue(parameterName, &ret);
        if (ret != 0) {
            parameterValue = std::string("");
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
                }
                else {
                    this->eraseParameter(parameterName);
                }

                if (this->nvsFlash != nullptr) {
                    delete this->nvsFlash;
                }
                this->nvsFlash = nullptr;
            }
        }

        resp_str += "<h3> Parameter " + parameterName + "</h3>";
        resp_str += "<input type=\"text\" name=\"" + parameterName + "\" id=\"" + parameterName + "\" size=\"100\" maxlength=\"200\" value=\"" + parameterValue + "\">";
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
        resp_str += "<br/>";
    }
    resp_str += "<button type=\"submit\">submit</button>";
    resp_str += "</form>";
    resp_str += "</body>";


    if (buf_len > 1) {
        free(buf);
    }

    httpd_resp_send(req, resp_str.c_str(), HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
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
    // Stop the httpd server
    return httpd_stop(server);
}

