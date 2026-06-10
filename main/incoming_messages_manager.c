#include <stdio.h>
#include "control_server.h"
#include "initialization_tasks.h"
#include "common_types.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = ANSI_COLOR_CYAN "Менеджер входящих сообщений" ANSI_COLOR_RESET;

void incoming_messages_manager(void *pvParameters)
{
    // получаем дескриптор очереди входящих сообщений, который был передан при создании задачи Менеджера входящих сообщений
    QueueHandle_t incoming_messages_queue = (QueueHandle_t)pvParameters;
    
    // буфер для входящего сообщения
    // ПОСЛЕ ПРОЧТЕНИЯ И ПАРСИНГА СООБЩЕНИЯ, НЕОБХОДИМО ОСВОБОДИТЬ ПАМЯТЬ !!!
    incoming_message_t message;
    
    // флаг для проверки, найден ли исполнительный модуль в списке executors_list
    bool found_executor;
    
    while (1) {

        message.data_ptr = NULL;
        if (xQueueReceive(incoming_messages_queue, &message, portMAX_DELAY) == pdTRUE) {

            // временная копия строки сообщения, которая будет изменяться функцией strtok_r, 
            //чтобы не потерять оригинальную строку для логирования
            char *copy_of_message = strdup(message.data_ptr);

            // получаем указатели на части сообщения, разделенные символом ":"
            char *saveptr;
            char *executor_id_token = strtok_r(message.data_ptr, ":", &saveptr);
            char *command_token = strtok_r(NULL, ":", &saveptr);
            char *parameter_token = strtok_r(NULL, "\r\n", &saveptr);

            // если исполнитель или команда не были получены
            // значит выводим ошибку, освобождаем память и пропускаем это сообщение
            if (executor_id_token == NULL || command_token == NULL) {
                ESP_LOGE(TAG, "Получено сообщение с неверным форматом: %s", copy_of_message);
                free(message.data_ptr);
                free(copy_of_message);
                continue;
            } 
            // освобождаем временную копию строки, так как она больше не нужна
            free(copy_of_message); 

            // структура для хранения и передачи распарсенной информации о команде и параметре в очередь исполнительного модуля
            incoming_command_info_t command_info;
            command_info.command_ptr = NULL;
            command_info.parameter_ptr = NULL;
            
            // создать копии команды и параметра в структуре command_info
            command_info.command_ptr = strdup(command_token);

            if (parameter_token != NULL) {
                command_info.parameter_ptr = strdup(parameter_token);
            } else {
                // если в сообщении нет параметра, то запишем в parameter_ptr пустую строку
                command_info.parameter_ptr = strdup("");
            }

            // если память не выделилась, то выводим ошибку, освобождаем память и пропускаем это сообщение
            if (command_info.command_ptr == NULL || command_info.parameter_ptr == NULL) {
                ESP_LOGE(TAG, "Не выделена память под команду или параметр.");
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
                ESP_LOGE(TAG, "Получено сообщение для неизвестного исполнителя: %s", executor_id_token);
                // раз команда и параметр не были отправлены, то освобождаем память, выделенную для них
                free(command_info.command_ptr);
                free(command_info.parameter_ptr);
            }
            
            // после обработки сообщения, освобождаем память, выделенную для него
            free(message.data_ptr);
        }
    }
}


