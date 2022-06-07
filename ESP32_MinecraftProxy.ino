#include <Preferences.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <AsyncUDP.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <SPIFFS.h>

#include "Config.h"
#include "ProxyServer.h"

#define min(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#define max(a, b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })

bool connected = false;
uint64_t bytes_transferred = 0;

void setup()
{
    Serial.begin(115200);
    Serial.printf("\n\n\n");

    Serial.printf("[i] SDK:          '%s'\n", ESP.getSdkVersion());
    Serial.printf("[i] CPU Speed:    %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("[i] Chip Id:      %06llX\n", ESP.getEfuseMac());
    Serial.printf("[i] Flash Mode:   %08X\n", ESP.getFlashChipMode());
    Serial.printf("[i] Flash Size:   %08X\n", ESP.getFlashChipSize());
    Serial.printf("[i] Flash Speed:  %d MHz\n", ESP.getFlashChipSpeed() / 1000000);
    Serial.printf("[i] Heap          %d/%d\n", ESP.getFreeHeap(), ESP.getHeapSize());
    Serial.printf("[i] SPIRam        %d/%d\n", ESP.getFreePsram(), ESP.getPsramSize());
    Serial.printf("\n");
    Serial.printf("[i] Starting\n");
    Serial.printf("[i]   Setup SPIFFS\n");
    if (!SPIFFS.begin(true))
    {
        Serial.println("[E]   SPIFFS Mount Failed");
    }
    
    cfg_read();
    current_config.current_entry++;;
    current_config.current_entry %= CONFIG_SERVER_COUNT;
    cfg_save();
    
    Serial.printf("[i]   Setup WiFi\n");
    wifi_setup();
    Serial.printf("[i]   Setup Webserver\n");
    www_setup();
    Serial.printf("[i]   Setup MQTT\n");
    mqtt_setup();
}

ProxyServer *CurrentProxy;

void loop()
{
    wifi_loop();
    www_loop();
    ota_loop();
    mqtt_loop();
    
  	if (WiFi.status() == WL_CONNECTED)
    {
        if(!connected)
        {
            connected = true;
            CurrentProxy = new ProxyServer(current_config.remote_name[current_config.current_entry], current_config.remote_server[current_config.current_entry], current_config.remote_port[current_config.current_entry], current_config.local_port);
        }
        CurrentProxy->Loop();
   }
   else
   {
   }
}
