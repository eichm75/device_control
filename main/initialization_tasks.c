#include "esp_log.h"
#include "common_types.h"
#include "executive_module_1.h"
#include "executive_module_2.h"
#include "incoming_messages_manager.h"
#include "control_server.h"

const char *TAG = "Инициализация задач";

// задание списка исполнительных модулей, их задач и очередей.
executor_config_t executors_list[] = {
    {
        .executor_id = "EM1",
        .executor_function = executive_module_1,
        .stack_size = 2048,
        .queue_length = 10,
        .executor_queue = NULL
    },
    {
        .executor_id = "EM2",
        .executor_function = executive_module_2,
        .stack_size = 2048,
        .queue_length = 10,
        .executor_queue = NULL
    }
};

// вычисляем количество исполнительных модулей в списке executors_list
const int EXECUTORS_COUNT = sizeof(executors_list) / sizeof(executor_config_t);

// функция для создания задач исполнительных модулей и их очередей
void start_executors_tasks(void)
{
// просмотреть весь список исполнительных модулей
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
}

// дескриптор очереди для Менеджера входящих сообщений
QueueHandle_t incoming_messages_queue;

// функция для создания задачи Менеджера входящих сообщений и его очереди
void start_incoming_messages_manager(void)
{
    // создаем очередь для входящих сообщений для Менеджера входящих команд.
    incoming_messages_queue = xQueueCreate(10, sizeof(incoming_message_t));

    // передать дескриптор очереди входящих сообщений в модуль control_server, чтобы он мог помещать в нее сообщения, полученные от клиентов через веб-сокеты
    set_incoming_messages_queue(incoming_messages_queue);


    // создаем задачу Менеджер входящих сообщений
    xTaskCreate(incoming_messages_manager, "incoming_messages_manager", 4096, incoming_messages_queue, 5, NULL);
}