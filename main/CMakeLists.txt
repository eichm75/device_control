idf_component_register(SRCS "feedback_messages_manager.c" "initialization_tasks.c" "incoming_messages_manager.c" "executive_module_2.c" "executive_module_1.c" "main.c" "control_server.c" "common_types.h"
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)
