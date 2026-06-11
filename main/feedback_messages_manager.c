#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "common_types.h"
#include "control_server.h"
#include "initialization_tasks.h"

static const char *TAG = "Менеджер сообщений обратной связи";


void feedback_messages_manager(void *pvParameters)
{
    // получаем дескриптор очереди сообщений обратной связи, который был передан при создании задачи Менеджера сообщений обратной связи
    QueueHandle_t feedback_messages_queue = (QueueHandle_t)pvParameters;
    
    // буфер для сообщения обратной связи
    feedback_message_t feedback_message;

    while (1) {
        feedback_message.data_ptr = NULL;
        if (xQueueReceive(feedback_messages_queue, &feedback_message, portMAX_DELAY) == pdTRUE) {
            // ЛОГИРУЕМ СООБЩЕНИЕ ОБРАТНОЙ СВЯЗИ
            ESP_LOGI(TAG, "Сообщение обратной связи: %s", feedback_message.data_ptr);
            
            // ОЧИСТКА ПАМЯТИ: Освобождаем память, выделенную для строки сообщения обратной связи
            free(feedback_message.data_ptr);
        }
    }
}