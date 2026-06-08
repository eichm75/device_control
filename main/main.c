#include <stdio.h>
#include "control_server.h"
#include "executors_list.h"
#include "common_types.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "Device Control Main";

// очередь для Менеджера входящих сообщений
QueueHandle_t incoming_messages_queue;

// задача Менеджер входящих сообщений, которая будет получать сообщения от управляющих модулей через очередь и распределять их 
// между исполнительными модулями
void incoming_messages_manager(void *pvParameters) {
    
    // буфер для хранения идентификатора источника сообщения
    //source_message_t source = UNKNOWN_SOURCE;
    
    // буфер для входящего сообщения
    incoming_message_t message;

    // буфер для хранения идентификатора исполнительного модуля
    char executor_id[EXECUTOR_ID_LENGTH]; 
    
    // структура для хранения распарсенной информации о команде и параметре
    incoming_command_info_t command_info;

    // флаг для проверки, найден ли исполнительный модуль в списке executors_list
    bool found_executor;
    
    while (1) {
        if (xQueueReceive(incoming_messages_queue, &message, portMAX_DELAY) == pdTRUE) {
            
            // извлекаем источник сообщения
            //source = message.source;
            
            // очищаем буферы для идентификатора исполнителя, команды и параметра перед копированием новых данных
            memset(executor_id, 0, sizeof(executor_id));
            memset(command_info.command, 0, sizeof(command_info.command));
            memset(command_info.parameter, 0, sizeof(command_info.parameter));
            
            char *saveptr;
            char *executor_id_token = strtok_r(message.data, ":", &saveptr);
            char *command_token = strtok_r(NULL, ":", &saveptr);
            char *parameter_token = strtok_r(NULL, "\r\n", &saveptr);

            if (executor_id_token == NULL || command_token == NULL) {
                char invalid_message[INCOMING_MESSAGE_DATA_LENGTH+1];
                strncpy(invalid_message, message.data, INCOMING_MESSAGE_DATA_LENGTH);
                invalid_message[INCOMING_MESSAGE_DATA_LENGTH] = '\0';
                ESP_LOGE("Менеджер входящих сообщений", "Получено сообщение с неверным форматом: %s", invalid_message);
                continue;
            }

            strncpy(command_info.command, command_token, COMMAND_LENGTH);
            if (parameter_token != NULL) {
                strncpy(command_info.parameter, parameter_token, PARAMETER_LENGTH);
            } else {
                command_info.parameter[0] = '\0'; // если параметр отсутствует, устанавливаем пустую строку
            }
  
            found_executor = false;

            // ищем очередь исполнителя в списке executors_list по его идентификатору executor_id
            for (int i=0; i < EXECUTORS_COUNT; i++) {
                if (strncmp(executor_id_token, executors_list[i].executor_id, EXECUTOR_ID_LENGTH) == 0) {
                    // помещаем распарсенную информацию о команде в очередь для соответствующего исполнительного модуля
                    xQueueSend(executors_list[i].executor_queue, &command_info, 0); 
                    // отметим, что модуль найден
                    found_executor = true;
                    break;
                }
            }

            if (!found_executor) {
                char unknown_executor_message[INCOMING_MESSAGE_DATA_LENGTH+1];
                strncpy(unknown_executor_message, message.data, INCOMING_MESSAGE_DATA_LENGTH);
                unknown_executor_message[INCOMING_MESSAGE_DATA_LENGTH] = '\0';
                ESP_LOGE("Менеджер входящих сообщений", "Получено сообщение для неизвестного исполнителя: %s", unknown_executor_message);
            }
        }
    }
}

void app_main(void)
{
    // инициализируем NVS, который будет использоваться для хранения данных конфигурации Wi-Fi и других параметров
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    // проверяем результат инициализации NVS и выводим сообщение об ошибке, если она произошла
    ESP_ERROR_CHECK(ret);

    // создаем задачи для исполнительных модулей и их очереди, используя список executors_list
    for (int i=0; i < EXECUTORS_COUNT; i++) {

        // создаем очередь для исполнительного модуля и сохраняем ее дескриптор в структуре executors_list
        executors_list[i].executor_queue = xQueueCreate(executors_list[i].queue_length, sizeof(incoming_command_info_t));
        if (executors_list[i].executor_queue == 0) {
            ESP_LOGE(TAG, "Не удалось создать очередь для исполнителя: %s", executors_list[i].executor_id);
            continue;
        }

        // создаем задачу исполнителя, передавая ему функцию и дескриптор очереди
        BaseType_t ret = xTaskCreate(
            executors_list[i].executor_function, 
            executors_list[i].executor_id, 
            executors_list[i].stack_size, 
            executors_list[i].executor_queue, 
            5, 
            NULL);
        if (ret == pdPASS) {
            ESP_LOGI(TAG, "Задача для исполнителя %s успешно создана", executors_list[i].executor_id);
        } else {
            ESP_LOGE(TAG, "Не удалось создать задачу для исполнителя: %s", executors_list[i].executor_id);
        }
    }

    // создаем очередь для входящих сообщений для Менеджера входящих команд.
    incoming_messages_queue = xQueueCreate(10, sizeof(incoming_message_t));

    // передать дескриптор очереди входящих сообщений в модуль control_server, чтобы он мог помещать в нее сообщения, полученные от клиентов через веб-сокеты
    set_incoming_messages_queue(incoming_messages_queue);


    // создаем задачу Менеджер входящих сообщений
    xTaskCreate(incoming_messages_manager, "incoming_messages_manager", 4096, NULL, 5, NULL);
    // запускаем сервер управления, который будет принимать управляющие команды через wifi
    start_control_server();
}
