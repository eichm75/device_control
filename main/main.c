#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "common_types.h"
#include "control_server.h"
#include "executors_list.h"
#include "incoming_messages_manager.h"

static const char *TAG = "Device Control Main";

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

    start_incoming_messages_manager();

    // запускаем сервер управления, который будет принимать управляющие команды через wifi
    start_control_server();
}
