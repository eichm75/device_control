#include "esp_log.h"
#include <string.h>
#include "common_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = ANSI_COLOR_BLUE "Исполнитель_2" ANSI_COLOR_RESET;

// задача Исполнительный модуль 2, которая будет получать команды от Менеджера входящих сообщений через очередь и выполнять их
void executive_module_2(void *pvParameters)
{

    // получить дескриптор очереди из параметров задачи
    QueueHandle_t incoming_commands_em2_queue = (QueueHandle_t)pvParameters;

    // структура для хранения распарсенной информации о команде, которая будет извлекаться из очереди
    incoming_command_info_t command_info;

    char command[COMMAND_LENGTH + 1];     // буфер для хранения команды
    char parameter[PARAMETER_LENGTH + 1]; // буфер для хранения параметра команды

    while (1)
    {
        if (xQueueReceive(incoming_commands_em2_queue, &command_info, portMAX_DELAY) == pdTRUE)
        {
            // очищаем буферы для команды и параметра перед копированием новых данных
            memset(command, 0, sizeof(command));
            memset(parameter, 0, sizeof(parameter));

            // копируем команду и параметр из структуры command_info в соответствующие буферы
            strncpy(command, command_info.command, COMMAND_LENGTH);
            command[COMMAND_LENGTH - 1] = '\0';
            strncpy(parameter, command_info.parameter, PARAMETER_LENGTH);
            parameter[PARAMETER_LENGTH - 1] = '\0';

            ESP_LOGI(TAG, ANSI_COLOR_BLUE "Получена команда: %s, с параметром: %s" ANSI_COLOR_RESET, command, parameter);
        }
    }
}