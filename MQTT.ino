#define MQTT_DEBUG

#include <PubSubClient.h>
#include <ESP32httpUpdate.h>


#define COMMAND_TOPIC "tele/mc_proxy/command"
#define RESPONSE_TOPIC "tele/mc_proxy/response"

WiFiClient client;
PubSubClient mqtt(client);

extern uint64_t bytes_transferred;

int mqtt_last_publish_time = 0;
int mqtt_lastConnect = 0;
int mqtt_retries = 0;
bool mqtt_fail = false;

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.print("'");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.print("'");
    Serial.println();

    payload[length] = 0;

    if(!strcmp(topic, COMMAND_TOPIC))
    {
      char *command = (char *)payload;
      
      if(!strncmp(command, "http", 4))
      {
          char buf[1024];
          sprintf(buf, "updating from: '%s'", command);
          Serial.printf("%s\n", buf);
          mqtt.publish(RESPONSE_TOPIC, buf);
          t_httpUpdate_return ret = ESPhttpUpdate.update(command);
          
          sprintf(buf, "update failed");
          switch(ret)
          {
              case HTTP_UPDATE_FAILED:
                  sprintf(buf, "HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                  break;
  
              case HTTP_UPDATE_NO_UPDATES:
                  sprintf(buf, "HTTP_UPDATE_NO_UPDATES");
                  break;
  
              case HTTP_UPDATE_OK:
                  sprintf(buf, "HTTP_UPDATE_OK");
                  break;
          }
          mqtt.publish(RESPONSE_TOPIC, buf);
          Serial.printf("%s\n", buf);
      }
      else
      {
          Serial.printf("unknown command: '%s'", command);
      }
    }
}

void mqtt_setup()
{
    mqtt.setServer(current_config.mqtt_server, current_config.mqtt_port);
    mqtt.setCallback(callback);
}

void mqtt_publish_float(char *name, float value)
{
    char buffer[32];

    sprintf(buffer, "%0.2f", value);
    if(false)
    {
    if (!mqtt.publish(name, buffer))
    {
        mqtt_fail = true;
    }
    }
    Serial.printf("Published %s : %s\n", name, buffer);
}

void mqtt_publish_int(char *name, uint32_t value)
{
    char buffer[32];

    if (value == 0x7FFFFFFF)
    {
        return;
    }
    sprintf(buffer, "%d", value);
    if (!mqtt.publish(name, buffer))
    {
        mqtt_fail = true;
    }
    Serial.printf("Published %s : %s\n", name, buffer);
}

bool mqtt_loop()
{
    uint32_t time = millis();
    static int nextTime = 0;

    if (mqtt_fail)
    {
        mqtt_fail = false;
        mqtt.disconnect();
    }

    MQTT_connect();

    if (!mqtt.connected())
    {
        return false;
    }

    mqtt.loop();

    if (time >= nextTime)
    {
        bool do_publish = false;

        if ((time - mqtt_last_publish_time) > 10000)
        {
            do_publish = true;
        }

        if (do_publish)
        {
            mqtt_last_publish_time = time;
            
            mqtt_publish_int((char*)"feeds/integer/mc_proxy/bytes_transferred", bytes_transferred);
            mqtt_publish_int((char*)"feeds/integer/mc_proxy/rssi", WiFi.RSSI());
        }
        nextTime = time + 1000;
    }

    return false;
}

void MQTT_connect()
{
    int curTime = millis();
    int8_t ret;

    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }

    if (mqtt.connected())
    {
        return;
    }

    if ((mqtt_lastConnect != 0) && (curTime - mqtt_lastConnect < (1000 << mqtt_retries)))
    {
        return;
    }

    mqtt_lastConnect = curTime;

    Serial.print("MQTT: Connecting to MQTT... ");
    ret = mqtt.connect(current_config.mqtt_client, current_config.mqtt_user, current_config.mqtt_password);

    if (ret == 0)
    {
        mqtt_retries++;
        if (mqtt_retries > 8)
        {
            mqtt_retries = 8;
        }
        Serial.printf("MQTT: (%d) ", mqtt.state());
        Serial.println("MQTT: Retrying MQTT connection");
        mqtt.disconnect();
    }
    else
    {
        Serial.println("MQTT Connected!");
        mqtt.subscribe(COMMAND_TOPIC);
    }
}
