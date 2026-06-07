#include "executors_list.h"
#include "common_types.h"
#include "executive_module_1.h"
#include "executive_module_2.h"

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