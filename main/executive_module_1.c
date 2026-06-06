#include "esp_log.h"
#include <string.h>
#include "common_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "Executive Module 1";
static QueueHandle_t incoming_commands_em1_queue;

// функция для получения дескриптора очереди входящих команд, объявленной в main.c
void set_incoming_commands_em1_queue(QueueHandle_t queue)
{
    incoming_commands_em1_queue = queue;
}

// задача Исполнительный модуль 1, которая будет получать команды от Менеджера входящих сообщений через очередь и выполнять их
void executive_module_1(void *pvParameters) {
    incoming_command_info_t command_info; // структура для хранения распарсенной информации о команде, которая будет извлекаться из очереди
    char command[COMMAND_LENGTH + 1]; // буфер для хранения команды
    char parameter[PARAMETER_LENGTH + 1]; // буфер для хранения параметра команды


    while (1) {
        if (xQueueReceive(incoming_commands_em1_queue, &command_info, portMAX_DELAY) == pdTRUE) {
            
            memset(command, 0, sizeof(command));
            memset(parameter, 0, sizeof(parameter));

            // копируем команду и параметры из структуры command_info в соответствующие буферы
            strncpy(command, command_info.command, COMMAND_LENGTH);
            command[COMMAND_LENGTH-1] = '\0';
            strncpy(parameter, command_info.parameter, PARAMETER_LENGTH);
            parameter[PARAMETER_LENGTH-1] = '\0';

            ESP_LOGI(TAG, ANSI_COLOR_BLUE"Получена команда: %s, с параметром: %s"ANSI_COLOR_RESET, command, parameter);
           
        }
    }
}