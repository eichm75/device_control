#include "esp_log.h"
#include <string.h>
#include "common_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = ANSI_COLOR_CYAN "Исполнитель_1" ANSI_COLOR_RESET;

// задача Исполнительный модуль 1, которая будет получать команды от Менеджера входящих сообщений через очередь и выполнять их
void executive_module_1(void *pvParameters)
{
    QueueHandle_t incoming_commands_em1_queue = (QueueHandle_t)pvParameters;
    incoming_command_info_t command_info;

    while (1)
    {
        if (xQueueReceive(incoming_commands_em1_queue, &command_info, portMAX_DELAY) == pdTRUE)
        {
            // РАБОТАЕМ НАПРЯМУЮ! Никаких calloc, strncpy и лишних free(command)
            ESP_LOGI(TAG, ANSI_COLOR_CYAN "Получена команда: %s, с параметром: %s" ANSI_COLOR_RESET, 
                     command_info.command_ptr, 
                     command_info.parameter_ptr);

            // Твоя будущая логика разбора команд через strcmp:
            if (strcmp(command_info.command_ptr, "SET_VOLUME") == 0) {
                // adau_set_volume(atoi(command_info.parameter_ptr));
            }

            // ОЧИСТКА ПАМЯТИ: Освобождаем ТО, ЧТО ВЫДЕЛЯЛ МЕНЕДЖЕР через strdup
            // Это должно быть строго ВНУТРИ блока if (xQueueReceive ... == pdTRUE),
            // чтобы мы не пытались удалить мусорные указатели, если очередь вернула ошибку.
            free(command_info.command_ptr);
            free(command_info.parameter_ptr);
        }
    }
}