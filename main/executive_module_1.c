#include "esp_log.h"
#include <string.h>

static const char *TAG = "Executive Module 1";
static QueueHandle_t incoming_commands_em1_queue;

// функция для получения дескриптора очереди входящих команд, объявленной в main.c
void set_incoming_commands_em1_queue(QueueHandle_t queue)
{
    incoming_commands_em1_queue = queue;
}