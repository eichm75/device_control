idf_component_register(SRCS "main.c" "control_server.c" "common_types.h"
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
