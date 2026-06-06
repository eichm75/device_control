#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void set_incoming_commands_em1_queue(QueueHandle_t queue);
void executive_module_1(void *pvParameters);