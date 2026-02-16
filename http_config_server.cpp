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

    if (this->configurationActive) {
        return false;
    }

    // Iterate parameters
    for (const auto& [parameterName, description] : this->parameters) {
        ESP_LOGI(this->tag.c_str(), "[%s] Description: \"%s\"", parameterName.c_str(), description.c_str());

        parameterValue = this->getStringParameterValue(parameterName, &ret);
        ESP_LOGI(this->tag.c_str(), "Parameter %s: getStringParameterValue ret: %u", parameterName.c_str(), ret);

        if (ret != ESP_OK) {
            allParametersConfigured = false;
        }
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
    return rc; }

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
    nvsFlash->SetStr(parameterName, parameterValue);

    return ESP_OK; // ToDo !
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

    // start HTTP server
}