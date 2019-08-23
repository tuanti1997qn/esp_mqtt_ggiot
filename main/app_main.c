#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include <time.h>
#include <sys/time.h>
#include "lwip/apps/sntp.h"

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "tcpip_adapter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"

#include "jwt.h"

static const char *TAG = "MQTTS_EXAMPLE";


#define MY_CLIENT_ID "projects/my-first-pj-249704/locations/asia-east1/registries/test_registry/devices/test-device-id"
#define CONFIG_ESP_WIFI_SSID "tuanti1997qn"
#define CONFIG_ESP_WIFI_PASSWORD "tuan123456"
#define MY_PJ_ID "my-first-pj-249704"
#define MY_ALGORITHM "ES256"
const char * MY_PRIVATE_CERT =
"-----BEGIN EC PRIVATE KEY-----"
"MHcCAQEEIO8CLb/2ycM+jzPi8uA8f+HX0+tygMMbdQe9Kv+Ha3BgoAoGCCqGSM49"
"AwEHoUQDQgAEkLtKS6qwaw1lFvkRRRm6P2LJMSKZp+gYfp+cgWBe3Pze5DiVzSqg"
"Av3lhXeuYsD6GVigfCIQ99Thi30KOORoag=="
"-----END EC PRIVATE KEY-----";


// char key[] = MY_PRIVATE_CERT;
size_t key_len = 0;

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t iot_eclipse_org_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t iot_eclipse_org_pem_start[]   asm("_binary_iot_eclipse_org_pem_start");
#endif
extern const uint8_t iot_eclipse_org_pem_end[]   asm("_binary_iot_eclipse_org_pem_end");

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

static void set_sys_time(void);
static void wifi_init(void);
static esp_err_t wifi_event_handler(void *ctx, system_event_t *event);
static void test_time( void );
static void mqtt_app_start(void);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);
static int GetAlgorithmFromString(const char *algorithm);
static char* CreateJwt(const char* project_id, const char *algorithm);
static int GetAlgorithmFromString(const char *algorithm) ;

time_t now = 0;
struct tm timeinfo = {0};
char strftime_buf[64];


void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init();
    set_sys_time();
    mqtt_app_start();
}


static void set_sys_time(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "time.google.com");
    sntp_init();
    // wait for time to be set
    // time_t now = 0;
    // struct tm timeinfo = {0};
    while (timeinfo.tm_year < (2018 - 1900))
    {
        ESP_LOGI(TAG, "Waiting for system time to be set...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);   
    }
    ESP_LOGI(TAG, "Time is set...");
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s]", CONFIG_ESP_WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "wifi done");
}



static void test_time( void )
{
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in New York is: %s", strftime_buf);
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error(event->error_handle, &mbedtls_err, NULL);
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URI,
        .cert_pem = (const char *)iot_eclipse_org_pem_start,
        .client_id = MY_CLIENT_ID,
        .username = "unused",
        .password = CreateJwt(MY_CLIENT_ID,MY_ALGORITHM),
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}





/**********************************************************************************/
static void GetIatExp(char* iat, char* exp, int time_size) 
{
  // TODO: Use time.google.com for iat
  time_t now_seconds = time(NULL);
  snprintf(iat, time_size, "%lu", now_seconds);
  snprintf(exp, time_size, "%lu", now_seconds + 3600);
    printf("IAT: %s\n", iat);
    printf("EXP: %s\n", exp);
}

static char* CreateJwt(const char* project_id, const char *algorithm) 
{
  char iat_time[sizeof(time_t) * 3 + 2];
  char exp_time[sizeof(time_t) * 3 + 2];
  char* key = NULL; // Stores the Base64 encoded certificate
  key_len = 0;
  jwt_t *jwt = NULL;
  int ret = 0;
  char *out = NULL;

  // Read private key from file
  // FILE *fp = fopen(ec_private_path, "r");
  // if (fp == (void*) NULL) {
  //   printf("Could not open file: %s\n", ec_private_path);
  //   return "";
  // }
  // fseek(fp, 0L, SEEK_END);
  // key_len = ftell(fp);
  // fseek(fp, 0L, SEEK_SET);
  key = malloc(sizeof(uint8_t) * (227)); // certificate length + \0
  strcpy(key,MY_PRIVATE_CERT);
  key_len = strlen(key);
  // fread(key, 1, key_len, fp);
  // key[key_len] = '\0';
  // fclose(fp);

  // Get JWT parts
  GetIatExp(iat_time, exp_time, sizeof(iat_time));

  jwt_new(&jwt);

  // Write JWT
  ret = jwt_add_grant(jwt, "iat", iat_time);
  if (ret) {
    printf("Error setting issue timestamp: %d\n", ret);
  }
  ret = jwt_add_grant(jwt, "exp", exp_time);
  if (ret) {
    printf("Error setting expiration: %d\n", ret);
  }
  ret = jwt_add_grant(jwt, "aud", project_id);
  if (ret) {
    printf("Error adding audience: %d\n", ret);
  }
  ret = jwt_set_alg(jwt, GetAlgorithmFromString(algorithm), key, key_len);
  if (ret) {
    printf("Error during set alg: %d\n", ret);
  }
  out = jwt_encode_str(jwt);
  if(!out) {
      perror("Error during token creation:");
  }
  // Print JWT
  // if (TRACE) {
  //   printf("JWT: [%s]\n", out);
  // }

  jwt_free(jwt);
  // free(key);
  return out;
}

static int GetAlgorithmFromString(const char *algorithm) 
{
    if (strcmp(algorithm, "RS256") == 0) {
        return JWT_ALG_RS256;
    }
    if (strcmp(algorithm, "ES256") == 0) {
        return JWT_ALG_ES256;
    }
    return -1;
}
