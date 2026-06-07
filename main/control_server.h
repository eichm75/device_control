#ifndef CONTROL_SERVER_H
#define CONTROL_SERVER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


void start_control_server(void);

void set_incoming_messages_queue(QueueHandle_t queue);

#endif // CONTROL_SERVER_H