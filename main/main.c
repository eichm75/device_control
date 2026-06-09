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
    // ПОСЛЕ ПРОЧТЕНИЯ И ПАРСИНГА СООБЩЕНИЯ, НЕОБХОДИМО ОСВОБОДИТЬ ПАМЯТЬ !!!
    incoming_message_t message;
    
    // флаг для проверки, найден ли исполнительный модуль в списке executors_list
    bool found_executor;
    
    while (1) {

        message.data_ptr = NULL;
        if (xQueueReceive(incoming_messages_queue, &message, portMAX_DELAY) == pdTRUE) {

            ESP_LOGI(TAG, "Получено новое сообщение для обработки. %s", message.data_ptr);

            // получаем указатели на части сообщения, разделенные символом ":"
            char *saveptr;
            char *executor_id_token = strtok_r(message.data_ptr, ":", &saveptr);
            char *command_token = strtok_r(NULL, ":", &saveptr);
            char *parameter_token = strtok_r(NULL, "\r\n", &saveptr);

            // если исполнитель или команда не были получены
            // значит выводим ошибку, освобождаем память и пропускаем это сообщение
            if (executor_id_token == NULL || command_token == NULL) {
                ESP_LOGE("Менеджер входящих сообщений", "Получено сообщение с неверным форматом: %s", message.data_ptr);
                free(message.data_ptr);
                continue;
            }

            // структура для хранения и передачи распарсенной информации о команде и параметре в очередь исполнительного модуля
            incoming_command_info_t command_info;
            command_info.command_ptr = NULL;
            command_info.parameter_ptr = NULL;
            
            command_info.command_ptr = strdup(command_token);

            if (parameter_token != NULL) {
                command_info.parameter_ptr = strdup(parameter_token);
            } else {
                command_info.parameter_ptr = strdup("");
            }

            // если память не выделилась, то выводим ошибку, освобождаем память и пропускаем это сообщение
            if (command_info.command_ptr == NULL || command_info.parameter_ptr == NULL) {
                ESP_LOGE("Менеджер входящих сообщений", "Не выделена память под команду или параметр.");
                free(command_info.command_ptr);
                free(command_info.parameter_ptr);
                free(message.data_ptr);
                continue;
            }

            found_executor = false;

            // ищем очередь исполнителя в списке executors_list по его идентификатору executor_id
            for (int i=0; i < EXECUTORS_COUNT; i++) {
                if (strcmp(executor_id_token, executors_list[i].executor_id) == 0) {
                    // помещаем распарсенную информацию о команде в очередь для соответствующего исполнительного модуля
                    xQueueSend(executors_list[i].executor_queue, &command_info, 0); 
                    // отметим, что модуль найден
                    found_executor = true;
                    break;
                }
            }

            if (!found_executor) {
                ESP_LOGE("Менеджер входящих сообщений", "Получено сообщение для неизвестного исполнителя: %s", executor_id_token);
                free(command_info.command_ptr);
                free(command_info.parameter_ptr);
            }
            
            // после обработки сообщения, освобождаем память, выделенную для него
            free(message.data_ptr);
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
