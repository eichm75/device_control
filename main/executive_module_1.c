#include "esp_log.h"
#include <string.h>
#include "common_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = ANSI_COLOR_CYAN "Исполнитель_1" ANSI_COLOR_RESET;

cmd_result_t handle_command1(const char *parameter) {
    // Логика обработки команды COMMAND1
    ESP_LOGI(TAG, ANSI_COLOR_BLUE "Обработка команды COMMAND1 с параметром: %s" ANSI_COLOR_RESET, parameter);
    return CMD_RES_OK;
}

cmd_result_t handle_command2(const char *parameter) {
    // Логика обработки команды COMMAND2
    ESP_LOGI(TAG, ANSI_COLOR_BLUE "Обработка команды COMMAND2 с параметром: %s" ANSI_COLOR_RESET, parameter);
    return CMD_RES_ERROR;
}

cmd_result_t handle_command3(const char *parameter) {
    // Логика обработки команды COMMAND3
    ESP_LOGI(TAG, ANSI_COLOR_BLUE "Обработка команды COMMAND3 с параметром: %s" ANSI_COLOR_RESET, parameter);
    return CMD_RES_ASYNC;
}

// Таблица команд и их функций-обработчиков для Исполнительного модуля 1
static const command_entry_t command_table[] = {
    {"COMMAND1", handle_command1},
    {"COMMAND2", handle_command2},
    {"COMMAND3", handle_command3},
};

// Макрос для получения количества команд в таблице
#define COMMAND_COUNT (sizeof(command_table) / sizeof(command_entry_t))


// задача Исполнительный модуль 1, которая будет получать команды от Менеджера входящих сообщений через очередь и выполнять их
void executive_module_1(void *pvParameters)
{
    QueueHandle_t incoming_commands_em1_queue = (QueueHandle_t)pvParameters;
    incoming_command_info_t command_info;

    while (1)
    {
        if (xQueueReceive(incoming_commands_em1_queue, &command_info, portMAX_DELAY) == pdTRUE)
        {
            // РАБОТАЕМ НАПРЯМУЮ! Никаких calloc, strncpy и лишних free(command)
            ESP_LOGI(TAG, ANSI_COLOR_CYAN "Получена команда: %s, с параметром: %s" ANSI_COLOR_RESET, 
                     command_info.command_ptr, 
                     command_info.parameter_ptr);

            bool command_found = false;
            for (size_t i = 0; i < COMMAND_COUNT; i++) {
                if (strcmp(command_info.command_ptr, command_table[i].command_name) == 0) {
                    // Команда найдена, вызываем ее обработчик
                    cmd_result_t result = command_table[i].handler(command_info.parameter_ptr);
                    if (result == CMD_RES_OK) {
                        ESP_LOGI(TAG, ANSI_COLOR_GREEN "Команда %s выполнена успешно." ANSI_COLOR_RESET, command_info.command_ptr);
                    } else if (result == CMD_RES_ERROR) {
                        ESP_LOGE(TAG, ANSI_COLOR_RED "Ошибка при выполнении команды %s." ANSI_COLOR_RESET, command_info.command_ptr);
                    } else if (result == CMD_RES_ASYNC) {
                        ESP_LOGI(TAG, ANSI_COLOR_YELLOW "Команда %s выполняется асинхронно." ANSI_COLOR_RESET, command_info.command_ptr);
                    }
                    command_found = true;
                    break;
                }
            }

            if (!command_found) {
                ESP_LOGE(TAG, ANSI_COLOR_RED "Неизвестная команда: %s" ANSI_COLOR_RESET, command_info.command_ptr);
            }

            // ОЧИСТКА ПАМЯТИ: Освобождаем ТО, ЧТО ВЫДЕЛЯЛ МЕНЕДЖЕР через strdup
            // Это должно быть строго ВНУТРИ блока if (xQueueReceive ... == pdTRUE),
            // чтобы мы не пытались удалить мусорные указатели, если очередь вернула ошибку.
            free(command_info.command_ptr);
            free(command_info.parameter_ptr);
        }
    }
}


