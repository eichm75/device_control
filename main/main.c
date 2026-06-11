#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "common_types.h"
#include "control_server.h"
#include "initialization_tasks.h"
#include "incoming_messages_manager.h"

//static const char *TAG = "Device Control Main";

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
    
    // запускаем задачи исполнительных модулей
    start_executors_tasks();

    // запускаем задачу Менеджера входящих сообщений
    start_incoming_messages_manager();

    // запускаем сервер управления, который будет принимать управляющие команды через wifi
    start_control_server();
}
