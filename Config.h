#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONFIG_MAGIC 0xE1AAFF00
#define CONFIG_SERVER_COUNT 2

#define CONFIG_SOFTAPNAME  "esp32-minecraft"
#define CONFIG_OTANAME     "MinecraftProxy"


typedef struct
{
    uint32_t magic;
    uint32_t verbose;
    uint32_t mqtt_publish;
    uint32_t current_entry;
    char hostname[32];
    char wifi_ssid[32];
    char wifi_password[32];
    char remote_name[CONFIG_SERVER_COUNT][64];
    char remote_server[CONFIG_SERVER_COUNT][64];
    int remote_port[CONFIG_SERVER_COUNT];
    int local_port;
    char mqtt_server[32];
    int mqtt_port;
    char mqtt_user[32];
    char mqtt_password[32];
    char mqtt_client[32];
} t_cfg;


extern t_cfg current_config;


#endif
