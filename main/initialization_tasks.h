#ifndef INITIALIZATION_TASKS_H
#define INITIALIZATION_TASKS_H

#include "common_types.h"

extern executor_config_t executors_list[];
extern const int EXECUTORS_COUNT;

extern QueueHandle_t incoming_messages_queue;

void start_executors_tasks(void);
void start_incoming_messages_manager(void);

#endif // INITIALIZATION_TASKS_H