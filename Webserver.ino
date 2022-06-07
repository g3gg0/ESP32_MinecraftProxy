
#include <WebServer.h>
#include <ESP32httpUpdate.h>
#include "Config.h"

WebServer webserver(80);
extern char wifi_error[];
extern bool wifi_captive;

int www_wifi_scanned = -1;


void www_setup()
{
    webserver.on("/", handle_root);
    webserver.on("/index.html", handle_index);
    webserver.on("/set_parm", handle_set_parm);
    webserver.on("/ota", handle_ota);
    webserver.on("/reset", handle_reset);
    webserver.on("/test", handle_test);
    webserver.onNotFound(handle_404);

    webserver.begin();
    Serial.println("[WWW] Webserver started");

    if (!MDNS.begin(current_config.hostname))
    {
        Serial.println("Error setting up MDNS responder!");
    }
    MDNS.addService("http", "tcp", 80);
}

bool www_loop()
{
    webserver.handleClient();
    return false;
}

void handle_root()
{
    webserver.send(200, "text/html", SendHTML());
}

void handle_404()
{
    if(wifi_captive)
    {
        char buf[128];
        sprintf(buf, "HTTP/1.1 302 Found\r\nContent-Type: text/html\r\nContent-length: 0\r\nLocation: http://%s/\r\n\r\n", WiFi.softAPIP().toString().c_str());
        webserver.sendContent(buf);
        Serial.printf("[WWW] 302 - http://%s%s/ -> http://%s/\n", webserver.hostHeader().c_str(), webserver.uri().c_str(), WiFi.softAPIP().toString().c_str());
    }
    else
    {
        webserver.send(404, "text/plain", "So empty here");
        Serial.printf("[WWW] 404 - http://%s%s/\n", webserver.hostHeader().c_str(), webserver.uri().c_str());
    }
}

void handle_index()
{
    webserver.send(200, "text/html", SendHTML());
}

void handle_ota()
{
    ota_setup();
    webserver.send(200, "text/html", SendHTML());
}

void handle_reset()
{
    webserver.send(200, "text/html", SendHTML());
    ESP.restart();
}

void handle_test()
{
    Serial.printf("Test\n");
    webserver.send(200, "text/html", SendHTML());
}

void handle_set_parm()
{
    current_config.mqtt_publish = MAX(0, MIN(65535, webserver.arg("mqtt_publish").toInt()));
    current_config.local_port = MAX(0, MIN(65535, webserver.arg("local_port").toInt()));
    
    current_config.verbose = 0;
    current_config.verbose |= (webserver.arg("verbose_c0") != "") ? 1 : 0;
    current_config.verbose |= (webserver.arg("verbose_c1") != "") ? 2 : 0;
    current_config.verbose |= (webserver.arg("verbose_c2") != "") ? 4 : 0;
    current_config.verbose |= (webserver.arg("verbose_c3") != "") ? 8 : 0;

    strncpy(current_config.hostname, webserver.arg("hostname").c_str(), sizeof(current_config.hostname));
    strncpy(current_config.wifi_ssid, webserver.arg("wifi_ssid").c_str(), sizeof(current_config.wifi_ssid));
    strncpy(current_config.wifi_password, webserver.arg("wifi_password").c_str(), sizeof(current_config.wifi_password));

    for(int num = 0; num < CONFIG_SERVER_COUNT; num++)
    {
        char tmp[32];
        sprintf(tmp, "remote_name_%d", num);
        strncpy(current_config.remote_name[num], webserver.arg(tmp).c_str(), sizeof(current_config.remote_name[num]));
        sprintf(tmp, "remote_server_%d", num);
        strncpy(current_config.remote_server[num], webserver.arg(tmp).c_str(), sizeof(current_config.remote_server[num]));
        sprintf(tmp, "remote_port_%d", num);
        current_config.remote_port[num] = MAX(0, MIN(65535, webserver.arg(tmp).toInt()));
    }

    cfg_save();

    if (current_config.verbose)
    {
        Serial.printf("Config:\n");
        Serial.printf("  mqtt_publish:     %d %%\n", current_config.mqtt_publish);
        Serial.printf("  verbose:          %d\n", current_config.verbose);
    }

    if(webserver.arg("http_update") != "")
    {
        webserver.send(200, "text/text", "Updating from " + webserver.arg("http_update"));
        
        Serial.println("updating from: " + webserver.arg("http_update"));
        t_httpUpdate_return ret = ESPhttpUpdate.update(webserver.arg("http_update"));

        switch(ret)
        {
            case HTTP_UPDATE_FAILED:
                Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                break;

            case HTTP_UPDATE_NO_UPDATES:
                Serial.println("HTTP_UPDATE_NO_UPDATES");
                break;

            case HTTP_UPDATE_OK:
                Serial.println("HTTP_UPDATE_OK");
                break;
        }
        return;
    }
    
    if(webserver.arg("reboot") == "true")
    {
        webserver.send(200, "text/html", "<html><head><meta http-equiv=\"Refresh\" content=\"5; url=/index.html\"/></head><body><h1>Saved. Rebooting...</h1>(will refresh page in 5 seconds)</body></html>");
        delay(500);
        ESP.restart();
        return; 
    }
    
    if(webserver.arg("scan") == "true")
    {
        www_wifi_scanned = WiFi.scanNetworks();
    }
    webserver.send(200, "text/html", SendHTML());
    www_wifi_scanned = -1;
}

String SendHTML()
{
    char buf[1024];

    String ptr = "<!DOCTYPE html> <html>\n";
    ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";

    sprintf(buf, "<title>MinecraftProxy Control</title>\n");

    ptr += buf;
    ptr += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
    ptr += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
    ptr += "input{font-family: Consolas,Monaco,Lucida Console,Liberation Mono,DejaVu Sans Mono,Bitstream Vera Sans Mono,Courier New, monospace; }\n";
    ptr += "tr:nth-child(odd) {background: #CCC} tr:nth-child(even) {background: #FFF}\n";
    ptr += ".button {display: block;width: 80px;background-color: #3498db;border: none;color: white;padding: 13px 30px;text-decoration: none;font-size: 25px;margin: 0px auto 35px;cursor: pointer;border-radius: 4px;}\n";
    ptr += ".button-on {background-color: #3498db;}\n";
    ptr += ".button-on:active {background-color: #2980b9;}\n";
    ptr += ".button-off {background-color: #34495e;}\n";
    ptr += ".button-off:active {background-color: #2c3e50;}\n";
    ptr += ".toggle-buttons input[type=\"radio\"] {visibility: hidden;}\n";
    ptr += ".toggle-buttons label { border: 1px solid #333; border-radius: 0.5em; padding: 0.3em; } \n";
    ptr += ".toggle-buttons input:checked + label { background: #40ff40; color: #116600; box-shadow: none; }\n";
    ptr += ".check-buttons input[type=\"checkbox\"] {visibility: hidden;}\n";
    ptr += ".check-buttons label { border: 1px solid #333; border-radius: 0.5em; padding: 0.3em; } \n";
    ptr += ".check-buttons input:checked + label { background: #40ff40; color: #116600; box-shadow: none; }\n";
    ptr += "input:hover + label, input:focus + label { background: #ff4040; } \n";
    ptr += ".together { position: relative; } \n";
    ptr += ".together input { position: absolute; width: 1px; height: 1px; top: 0; left: 0; } \n";
    ptr += ".together label { margin: 0.5em 0; border-radius: 0; } \n";
    ptr += ".together label:first-of-type { border-radius: 0.5em 0 0 0.5em; } \n";
    ptr += ".together label:last-of-type { border-radius: 0 0.5em 0.5em 0; } \n";
    ptr += "p {font-size: 14px;color: #888;margin-bottom: 10px;}\n"; 
    ptr += "td {padding: 0.3em}\n";
    ptr += "</style>\n";
    ptr += "</head>\n";
    ptr += "<body>\n";

    sprintf(buf, "<h1>MinecraftProxy</h1>\n");

    if(strlen(wifi_error) != 0)
    {
      sprintf(buf, "<h2>WiFi Error: %s</h1>\n", wifi_error);
    }

    ptr += buf;
    if (!ota_enabled())
    {
        ptr += "<a href=\"/ota\">[Enable OTA]</a> ";
    }
    ptr += "<br><br>\n";

    ptr += "<form id=\"config\" action=\"/set_parm\">\n";
    ptr += "<table>";

#define ADD_CONFIG(name, value, fmt, desc)                                                                                \
    do                                                                                                                    \
    {                                                                                                                     \
        ptr += "<tr><td><label for=\"" name "\">" desc ":</label></td>";                                                  \
        sprintf(buf, "<td><input type=\"text\" id=\"" name "\" name=\"" name "\" value=\"" fmt "\"></td></tr>\n", value); \
        ptr += buf;                                                                                                       \
    } while (0)

#define ADD_CONFIG_CHECK4(name, value, fmt, desc, text0, text1, text2, text3) \
    do \
    { \
        ptr += "<tr><td>" desc ":</td><td><div class=\"check-buttons together\">"; \
        sprintf(buf, "<input type=\"checkbox\" id=\"" name "_c0\" name=\"" name "_c0\" value=\"1\" %s>\n", (value&1)?"checked":""); \
        ptr += buf; \
        sprintf(buf, "<label for=\"" name "_c0\">" text0 "</label>\n"); \
        ptr += buf; \
        sprintf(buf, "<input type=\"checkbox\" id=\"" name "_c1\" name=\"" name "_c1\" value=\"1\" %s>\n", (value&2)?"checked":""); \
        ptr += buf; \
        sprintf(buf, "<label for=\"" name "_c1\">" text1 "</label>\n"); \
        ptr += buf; \
        sprintf(buf, "<input type=\"checkbox\" id=\"" name "_c2\" name=\"" name "_c2\" value=\"1\" %s>\n", (value&4)?"checked":""); \
        ptr += buf; \
        sprintf(buf, "<label for=\"" name "_c2\">" text2 "</label>\n"); \
        ptr += buf; \
        sprintf(buf, "<input type=\"checkbox\" id=\"" name "_c3\" name=\"" name "_c3\" value=\"1\" %s>\n", (value&8)?"checked":""); \
        ptr += buf; \
        sprintf(buf, "<label for=\"" name "_c3\">" text3 "</label>\n"); \
        ptr += buf; \
        sprintf(buf, "</div></td></tr>\n"); \
        ptr += buf; \
    } while (0)

#define ADD_CONFIG_COLOR(name, value, fmt, desc) \
    do \
    { \
        ptr += "<tr><td><label for=\"" name "\">" desc ":</label></td>"; \
        sprintf(buf, "<td><input type=\"text\" id=\"" name "\" name=\"" name "\" value=\"" fmt "\" data-coloris></td></tr>\n", value); \
        ptr += buf; \
    } while (0)

    ADD_CONFIG("hostname", current_config.hostname, "%s", "Hostname");
    
    ADD_CONFIG("wifi_ssid", current_config.wifi_ssid, "%s", "WiFi SSID");
    ADD_CONFIG("wifi_password", current_config.wifi_password, "%s", "WiFi Password");


    ptr += "<tr><td>WiFi networks:</td><td>";

    if (www_wifi_scanned == -1)
    {
        ptr += "<button type=\"submit\" name=\"scan\" value=\"true\">Scan WiFi</button>";
    }
    else if (www_wifi_scanned == 0)
    {
        ptr += "No networks found, <button type=\"submit\" name=\"scan\" value=\"true\">Rescan WiFi</button>";
    }
    else 
    {
        ptr += "<table>";
        ptr += "<tr><td><button type=\"submit\" name=\"scan\" value=\"true\">Rescan WiFi</button></td></tr>";
        for (int i = 0; i < www_wifi_scanned; ++i)
        {
            if(WiFi.SSID(i) != "")
            {
                ptr += "<tr><td align=\"left\"><tt><a href=\"javascript:void(0);\" onclick=\"document.getElementById('wifi_ssid').value = '";
                ptr += WiFi.SSID(i);
                ptr += "'\">";
                ptr += WiFi.SSID(i);
                ptr += "</a></tt></td><td align=\"left\"><tt>";
                ptr += WiFi.RSSI(i);
                ptr += " dBm</tt></td></tr>";
            }
        }
        ptr += "</table>";
    }

    ptr += "</td></tr>";
    
    
    ADD_CONFIG("local_port", current_config.local_port, "%d", "Local UDP port");
    for(int num = 0; num < CONFIG_SERVER_COUNT; num++)
    {
        char tmp[64];
        sprintf(tmp, "<tr><td><label for=\"remote_name_%d\">Server name #%d:</label></td>", num, num+1);
        ptr += tmp;
        sprintf(buf, "<td><input type=\"text\" id=\"remote_name_%d\" name=\"remote_name_%d\" value=\"%s\"></td></tr>\n", num, num, current_config.remote_name[num]);
        ptr += buf;
        
        sprintf(tmp, "<tr><td><label for=\"remote_server_%d\">Server address #%d:</label></td>", num, num+1);
        ptr += tmp;
        sprintf(buf, "<td><input type=\"text\" id=\"remote_server_%d\" name=\"remote_server_%d\" value=\"%s\"></td></tr>\n", num, num, current_config.remote_server[num]);
        ptr += buf;
        
        sprintf(tmp, "<tr><td><label for=\"remote_port_%d\">Server port #%d:</label></td>", num, num+1);
        ptr += tmp;
        sprintf(buf, "<td><input type=\"text\" id=\"remote_port_%d\" name=\"remote_port_%d\" value=\"%d\"></td></tr>\n", num, num, current_config.remote_port[num]);
        ptr += buf;
    }
    
    //ADD_CONFIG_CHECK4("verbose", current_config.verbose, "%d", "Verbosity", "Serial", "_", "_", "_");
    //ADD_CONFIG_CHECK4("mqtt_publish", current_config.mqtt_publish, "%d", "MQTT publishes", "RSSI", "Debug", "_", "_");

    
    ADD_CONFIG("http_update", "", "%s", "Update URL");

    ptr += "<td></td><td><input type=\"submit\" value=\"Save\"><button type=\"submit\" name=\"reboot\" value=\"true\">Save &amp; Reboot</button></td></table></form>\n";

    ptr += "</body>\n";
    ptr += "</html>\n";
    return ptr;
}
