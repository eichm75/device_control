#include <stdio.h>
#include "control_server.h"
#include "common_types.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "Device Control Main";

QueueHandle_t incoming_messages_queue;
QueueHandle_t incoming_commands_em1_queue;

char* get_source_name(source_message_t source) {
    switch(source) {
        case WEB_SERVER: return "WEB_SERVER";
        case CONTROL_PANEL: return "CONTROL_PANEL";
        case UNKNOWN_SOURCE: return "UNKNOWN_SOURCE";
        default: return "UNKNOWN";
    }
}

// задача Менеджер входящих сообщений, которая будет получать сообщения от управляющих модулей через очередь и распределять их 
// между исполнительными модулями
void incoming_messages_manager(void *pvParameters) {
    source_message_t source = UNKNOWN_SOURCE; // переменная для хранения источника сообщения, которая будет извлекаться из структуры incoming_message_t при получении сообщения из очереди
    incoming_message_t message;
    
    char executor_id[EXECUTOR_ID_LENGTH + 1]; // буфер для хранения идентификатора исполнительного модуля
    //char command_block[COMMAND_LENGTH + PARAMETERS_LENGTH + 2]; // буфер для хранения пары "команда + параметр"
    char command[COMMAND_LENGTH + 1]; // буфер для хранения команды
    char parameters[PARAMETERS_LENGTH + 1]; // буфер для хранения параметров команды


    while (1) {
        if (xQueueReceive(incoming_messages_queue, &message, portMAX_DELAY) == pdTRUE) {
            // извлекаем источник сообщения
            source = message.source;
            memset(executor_id, 0, sizeof(executor_id));
            memset(command, 0, sizeof(command));
            memset(parameters, 0, sizeof(parameters));
            // копируем первые EXECUTOR_ID_LENGTH символов из данных сообщения в буфер executor_id
            strncpy(executor_id, message.data, EXECUTOR_ID_LENGTH); 
            executor_id[sizeof(executor_id) - 1] = '\0';
            // извлекаем команду и параметры из данных сообщения, используя форматирование строки
            sscanf(message.data + (EXECUTOR_ID_LENGTH + 1), "%[^:]:%s", command, parameters); 
            command[sizeof(command) - 1] = '\0';
            parameters[sizeof(parameters) - 1] = '\0';
        }
        ESP_LOGI(TAG, ANSI_COLOR_BLUE"Поступило сообщение от источника %s: executor_id=%s, command=%s, parameters=%s"ANSI_COLOR_RESET, get_source_name(source), executor_id, command, parameters);
    
    }
}

void app_main(void)
{
    //esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // инициализируем NVS, который будет использоваться для хранения данных конфигурации Wi-Fi и других параметров
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    // проверяем результат инициализации NVS и выводим сообщение об ошибке, если она произошла
    ESP_ERROR_CHECK(ret);

    // создаем очередь для входящих сообщений для Менеджера входящих команд.
    incoming_messages_queue = xQueueCreate(10, sizeof(incoming_message_t));
    incoming_commands_em1_queue = xQueueCreate(10, sizeof(incoming_commands_info_t));

    // передать дескриптор очереди входящих сообщений в модуль control_server, чтобы он мог помещать в нее сообщения, полученные от клиентов через веб-сокеты
    set_incoming_messages_queue(incoming_messages_queue);
    // передать дескриптор очереди входящих команд в исполнительный модуль 1, чтобы он мог получать команды, предназначенные для него, из этой очереди
    set_incoming_commands_em1_queue(incoming_commands_em1_queue);
    // создаем задачу Менеджер входящих сообщений, которая будет распределять входящие сообщения от модулей управления устройством и распределять их
    // между исполнительными модулями
    xTaskCreate(incoming_messages_manager, "incoming_messages_manager", 4096, NULL, 5, NULL);
    // запускаем сервер управления, который будет принимать управляющие команды через wifi
    start_control_server();
}
