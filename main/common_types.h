/*
    файл содержит глобальные типы данных и константы, которые используются в других файлах проекта.
*/

// источник поступившего сообщения
typedef enum {
    WEB_SERVER,
    CONTROL_PANEL
} source_message_t;

// структура для хранения информации о сообщении, которая включает в себя источник сообщения, идентификатор клиента и данные сообщения
typedef struct {
    source_message_t source; // источник сообщения
    int client_id;
    char data[64]; // данные сообщения
} incoming_message_t;

#define ANSI_COLOR_BLUE    "\033[0;34m"
#define ANSI_COLOR_RESET   "\033[0m"