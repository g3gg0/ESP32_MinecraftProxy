
#include <FS.h>
#include <SPIFFS.h>

#include "Config.h"

t_cfg current_config;

void cfg_save()
{
    File file = SPIFFS.open("/config.dat", "w");
    if (!file || file.isDirectory())
    {
        return;
    }

    if (strlen(current_config.hostname) < 2)
    {
        strcpy(current_config.hostname, "MinecraftProxy");
    }

    file.write((uint8_t *)&current_config, sizeof(current_config));
    file.close();
}

void cfg_reset()
{
    memset(&current_config, 0x00, sizeof(current_config));

    current_config.magic = CONFIG_MAGIC;
    strcpy(current_config.hostname, "MinecraftProxy");
    
    strcpy(current_config.mqtt_server, "(not set)");
    current_config.mqtt_port = 11883;
    strcpy(current_config.mqtt_user, "(not set)");
    strcpy(current_config.mqtt_password, "(not set)");
    strcpy(current_config.mqtt_client, "MinecraftProxy");
    
    strcpy(current_config.wifi_ssid, "(not set)");
    strcpy(current_config.wifi_password, "(not set)");
    
    strcpy(current_config.remote_name[0], "Creative");
    strcpy(current_config.remote_server[0], "<IP>");
    current_config.remote_port[0] = 19132;
    
    strcpy(current_config.remote_name[1], "Survival");
    strcpy(current_config.remote_server[1], "<IP>");
    current_config.remote_port[1] = 19134;
    
    current_config.local_port = 19132;
    
    current_config.current_entry = 0;
    
    current_config.verbose = 0;
    current_config.mqtt_publish = 0;

    cfg_save();
}

void cfg_read()
{
    File file = SPIFFS.open("/config.dat", "r");

    if (!file || file.isDirectory())
    {
        cfg_reset();
    }
    else
    {
        file.read((uint8_t *)&current_config, sizeof(current_config));
        file.close();

        if (current_config.magic != CONFIG_MAGIC)
        {
            cfg_reset();
        }
    }
}
