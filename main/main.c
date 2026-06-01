#include <stdio.h>
#include "control_server.h"
#include "common_types.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "Device Control Main";

QueueHandle_t incoming_messages_queue;
QueueHandle_t incoming_commands_em1_queue;
QueueHandle_t incoming_commands_em2_queue;

// задача Менеджер входящих сообщений, которая будет получать сообщения от управляющих модулей через очередь и распределять их 
// между исполнительными модулями
void incoming_messages_manager(void *pvParameters) {
    
    incoming_message_t message;

    char executor_id[EXECUTOR_ID_LENGTH + 1]; // буфер для хранения идентификатора исполнительного модуля
    //char command_block[COMMAND_LENGTH + PARAMETERS_LENGTH + 2]; // буфер для хранения пары "команда + параметр"
    char command[COMMAND_LENGTH + 1]; // буфер для хранения команды
    char parameters[PARAMETERS_LENGTH + 1]; // буфер для хранения параметров команды


    while (1) {
        if (xQueueReceive(incoming_messages_queue, &message, portMAX_DELAY) == pdTRUE) {
            // извлекаем идентификатор исполнительного модуля из данных сообщения
            strncpy(executor_id, message.data, EXECUTOR_ID_LENGTH); // копируем первые EXECUTOR_ID_LENGTH символов из данных сообщения в буфер executor_id
            executor_id[sizeof(executor_id) - 1] = '\0';

            sscanf(message.data + (EXECUTOR_ID_LENGTH + 1), "%[^:]:%s", command, parameters); // извлекаем команду и параметры 
            // из данных сообщения, используя форматирование строки
            command[sizeof(command) - 1] = '\0'; // гарантируем, что строка команды заканчивается нулевым символом
            parameters[sizeof(parameters) - 1] = '\0'; // гарантируем, чтострока параметров заканчивается нулевым символом
            // извлекаем команду из данных сообщения
            //strncpy(command_block, message.data + (EXECUTOR_ID_LENGTH+1), (COMMAND_LENGTH + PARAMETERS_LENGTH + 1));
            //command_block[sizeof(command_block) - 1] = '\0';
        }
        ESP_LOGI(TAG, ANSI_COLOR_BLUE"Поступило сообщение от клиента %d: executor_id=%s, command=%s, parameters=%s"ANSI_COLOR_RESET, message.client_id, executor_id, command, parameters);
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

    // создаем очередь для входящих сообщений от модулей управления устройством
    incoming_messages_queue = xQueueCreate(10, sizeof(incoming_message_t));

    // передать дескриптор очереди входящих сообщений в модуль control_server, чтобы он мог помещать в нее сообщения, полученные от клиентов через веб-сокеты
    set_incoming_messages_queue(incoming_messages_queue);
    // создаем задачу Менеджер входящих сообщений, которая будет распределять входящие сообщения от модулей управления устройством и распределять их
    // между исполнительными модулями
    xTaskCreate(incoming_messages_manager, "incoming_messages_manager", 4096, NULL, 5, NULL);
    // запускаем сервер управления, который будет принимать управляющие команды через wifi
    start_control_server();
}
