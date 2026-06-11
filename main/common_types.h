// файл содержит определения общих типов данных.

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"

// тип данных для передачи управляющих сообщений от модулей управления устройством в Менеджер входящих сообщений. 
typedef struct {
    char *data_ptr; // указатель на принятые данные
} incoming_message_t;

// тип данных для передачи распарсенной информации о команде от Менеджера входящих сообщений в исполнительные модули.
typedef struct {
    char *command_ptr;
    char *parameter_ptr;
} incoming_command_info_t;

typedef struct {
    char *data_ptr; // указатель на строку сообщения обратной связи
} feedback_message_t;

// тип данных для указателя на функцию задачи FreeRTOS, которая будет выполняться исполнительным модулем.
typedef void (*task_function_t)(void *pvParameters); 

// структура для хранения конфигурации исполнительного модуля 
typedef struct {
    char *executor_id; // идентификатор исполнительного модуля
    task_function_t executor_function; // указатель на функцию задачи, соответствующую идентификатору исполнителя
    uint32_t stack_size; // размер стека для задачи исполнительного модуля  
    UBaseType_t queue_length; // длина очереди для задач исполнительного модуля
    QueueHandle_t executor_queue; // указатель на очередь команд для задачи исполнителя
} executor_config_t;

// возврат функции-обработчика команды
typedef enum {
    CMD_RES_OK = 0,
    CMD_RES_ERROR,
    CMD_RES_ASYNC
} cmd_result_t;

// указатель на функцию-обработчик команды, которая принимает строку параметра и возвращает результат выполнения команды.
typedef cmd_result_t (*command_handler_t)(const char *parameter); 

// структура хранения имени команды и ее функции-обработчика
typedef struct {
    const char *command_name; // имя команды, например "SET_VOLUME"
    command_handler_t handler; // указатель на функцию-обработчик команды
} command_entry_t;





// ANSI escape codes для цветного вывода в терминале
#define ANSI_COLOR_RESET   "\033[0m"
#define ANSI_COLOR_BLACK   "\033[0;30m"
#define ANSI_COLOR_RED     "\033[0;31m"
#define ANSI_COLOR_GREEN   "\033[0;32m"
#define ANSI_COLOR_YELLOW  "\033[0;33m"
#define ANSI_COLOR_BLUE    "\033[0;34m"
#define ANSI_COLOR_MAGENTA "\033[0;35m"
#define ANSI_COLOR_CYAN    "\033[0;36m"
#define ANSI_COLOR_WHITE   "\033[0;37m"

#endif // COMMON_TYPES_H