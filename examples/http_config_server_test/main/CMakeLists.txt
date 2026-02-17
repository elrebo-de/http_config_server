idf_component_register( SRCS "main.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES "elrebo-de__http_config_server"
                                 "elrebo-de__generic_nvsflash"
                                 "elrebo-de__wifi_manager"
                                 "esp_http_server"
                      )
