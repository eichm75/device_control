#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include <esp_http_server.h>

#include "lwip/err.h"
#include "lwip/sys.h"

#include "common_types.h"

httpd_handle_t server = NULL;

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

static const char *TAG = "Control Server";
static QueueHandle_t incoming_messages_queue;

// функция получения дескриптора очереди входящих сообщений, объявленной в main.c
void set_incoming_messages_queue(QueueHandle_t queue)
{
    incoming_messages_queue = queue;
}

// функция для инициализации SPIFFS, которая настраивает параметры файловой системы, регистрирует ее и выводит информацию о размере и использовании раздела SPIFFS в лог
void init_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS Partition size: total: %d, used: %d", total, used);
    }
}

// функция для обработки событий Wi-Fi, которая регистрирует события подключения и отключения клиентов от точки доступа и выводит информацию о них в лог
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

// функция для инициализации Wi-Fi в режиме точки доступа, которая настраивает параметры точки доступа, такие как SSID, пароль, 
// канал и максимальное количество подключений, и запускает Wi-Fi
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

// функция для обработки GET запроса на корневой URI "/", которая читает файл index.html из SPIFFS и отправляет его содержимое клиенту
esp_err_t index_get_handler(httpd_req_t *req)
{
    // открываем файл index.html для чтения
    FILE* f= fopen("/spiffs/index.html", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open index.html");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    // устанавливаем тип содержимого ответа как "text/html"
    httpd_resp_set_type(req, "text/html");
    // line - буфер для чтения строк из файла index.html
    char line[128];
    // читаем файл index.html построчно и отправляем каждую строку клиенту с помощью функции httpd_resp_sendstr_chunk
    while (fgets(line,sizeof(line), f)) {
        httpd_resp_sendstr_chunk(req, line);
    }
    fclose(f);
    // отправляем пустой чанк для обозначения конца ответа
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

// функция для обработки запросов на URI "/ws", которая обрабатывает рукопожатие при подключении клиента к веб-сокет серверу и принимает кадры данных от клиента, 
// помещая их в очередь входящих сообщений для дальнейшей обработки исполнительными модулями. *req - указатель на структуру httpd_req_t, которая содержит информацию 
// о запросе от клиента, такую как метод запроса, URI и данные запроса. функция возвращает esp_err_t, который указывает на результат 
// обработки запроса (ESP_OK для успешной обработки или ESP_FAIL для ошибки)
esp_err_t handle_ws_req(httpd_req_t *req)
{
    //структура для передачи сообщения в очередь Менеджера входящих сообщений
    incoming_message_t msg;
    msg.data_ptr = NULL;

    // если метод запроса - GET, то это означает, что клиент только что подключился к веб-сокет серверу и завершил рукопожатие, 
    // поэтому мы выводим сообщение в лог и возвращаем ESP_OK
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Новое соединение установлено.");
        return ESP_OK;
    }

    // кадр данных websocket
    httpd_ws_frame_t ws_pkt;
    // инициализировать все поля нулями
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    // устанавливаем тип данных из кадра как текстовый
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    // узнаем длину полученного сообщения
    // вызов функции с параметром max_len = 0, даст реальную длину данных в ws_pkt.len
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось получить размер кадра: %d", ret);
        return ret;
    }

    if (ws_pkt.len > 0) {
        // выделить память для полученных данных + 1 байт для \0, при выделении памяти, весь буфер будет заполнен нулями
        msg.data_ptr = heap_caps_calloc(1, ws_pkt.len + 1, MALLOC_CAP_SPIRAM);

        if (msg.data_ptr == NULL) {
            ESP_LOGE(TAG,"Не выделена память под входящее сообщение.");
            return ESP_FAIL;
        }
        // передать указатель на буфер в структуру ws_pkt
        ws_pkt.payload = msg.data_ptr;
        // принимаем сообщение в буфер 
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);

        // если произошла ошибка, то вывести предупреждение, освободить память и завершить функцию с ошибкой
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Не удалось получить данные: %d", ret);
            free(msg.data_ptr);
            return ret;
        }

        // поместить сообщение в очередь Менеджера входящих сообщений
        xQueueSend(incoming_messages_queue, &msg, 0); 
    }
    return ESP_OK;
}

// функция для настройки и запуска веб-сокет сервера, которая регистрирует обработчики URI для корневого URI "/" и URI "/ws", и возвращает дескриптор сервера
httpd_handle_t setup_websocket_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_get_handler,
        .user_ctx = NULL};

    httpd_uri_t ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = handle_ws_req,
        .user_ctx = NULL,
        .is_websocket = true};

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &ws);
    }

    return server;
}

void start_control_server(void)
{
    // инициализируем SPIFFS, который будет использоваться для хранения файлов веб-сокет сервера
    init_spiffs();
    // инициализируем Wi-Fi в режиме точки доступа, который будет использоваться для подключения клиентов к веб-сокет серверу
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    // функция для инициализации Wi-Fi в режиме точки доступа
    wifi_init_softap();
    // настраиваем и запускаем веб-сокет сервер, который будет обрабатывать запросы от клиентов и отправлять им данные через веб-сокеты
    setup_websocket_server();
}