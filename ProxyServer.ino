
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"

#include "ProxyServer.h"
#include "Config.h"

char packet_string[] = "MCPE;status;-1;1.0;0;0;0;(c) g3gg0.de;Creative;1;55555;";
uint8_t packet_magic[] = {0x00, 0xff, 0xff, 0x00, 0xfe, 0xfe, 0xfe, 0xfe, 0xfd, 0xfd, 0xfd, 0xfd, 0x12, 0x34, 0x56, 0x78};
  
void proxy_client_received(void *arg, AsyncUDPPacket& packet)
{
    ProxyServer *srv = (ProxyServer *)arg;
    srv->ClientReceived(packet);
}   
void proxy_server_received(void *arg, AsyncUDPPacket& packet)
{
    ProxyServer *srv = (ProxyServer *)arg;
    srv->ServerReceived(packet);
}

ProxyServer::ProxyServer (char *server_name, char *server_address, int server_port, int local_port)
{
    ServerName = strdup(server_name);
    ServerAddress = strdup(server_address);
    ServerPort = server_port;
    LocalPort = local_port;
    
    ClientPort = -1;
    proxy_out_port = local_port + 16384;
    
    Serial.printf("[Proxy] Set up '%s'\n", server_name);
    Serial.printf("[Proxy] Listening on   %s:%d\n", WiFi.localIP().toString().c_str(), local_port);
    Serial.printf("[Proxy] Forwarding to  %s:%d\n", server_address, server_port);
        
    if(!client_udp.listen(local_port))
    {
      Serial.printf("[Proxy] Failed to bind to %s:%d\n", WiFi.localIP(), local_port);
      return;
    }
    if(!server_udp.listen(proxy_out_port))
    {
      Serial.printf("[Proxy] Failed to bind to %s:%d\n", WiFi.localIP(), proxy_out_port);
      return;
    }

    proxy_server_addr.fromString(server_address);
    
    client_udp.onPacket(proxy_client_received, this);
    server_udp.onPacket(proxy_server_received, this);
}

void ProxyServer::Loop()
{
  static long nextChange = 0;

  if(millis() > nextChange)
  {
    nextChange = millis() + 1000;
  }
}

int ProxyServer::AssembleString(t_server_string *server_str, uint8_t *buf)
{
    uint8_t *str = (uint8_t *) server_str;
    uint16_t pos = 0;
    int field_num = 0;
    int field_pos = 0;
  
    while(field_num < STR_FIELD_COUNT)
    {
        if(str[field_num * STR_FIELD_SIZE + field_pos] == 0)
        {
            field_num++;
            field_pos = 0;
            if(field_num < STR_FIELD_COUNT)
            {
              buf[pos++] = ';';
            }
        }
        else
        {
            buf[pos++] = str[field_num * STR_FIELD_SIZE + field_pos];
            field_pos++;
        }
    }
  
    return pos;
}

t_server_string *ProxyServer::ParseString(uint8_t *string, int length)
{
    uint8_t *str = (uint8_t *)malloc(sizeof(t_server_string));
    int pos = 0;
    int field_num = 0;
    int field_pos = 0;

    memset(str, 0x00, sizeof(t_server_string));

    while(pos < length && field_num < STR_FIELD_COUNT)
    {
        if(string[pos] == ';')
        {
            field_num++;
            field_pos = 0;
            if(field_num < STR_FIELD_COUNT)
            {
              str[field_num * STR_FIELD_SIZE + field_pos] = 0;
            }
        }
        else
        {
            if(field_pos < STR_FIELD_SIZE - 1)
            {
              str[field_num * STR_FIELD_SIZE + field_pos] = string[pos];
              field_pos++;
              str[field_num * STR_FIELD_SIZE + field_pos] = 0;
            }
        }
        pos++;
    }

    return (t_server_string *) str;
}

void ProxyServer::SendPacket(IPAddress dst, uint16_t port)
{
  /* assume a certain server string length */
  int string_len = 1024;
  uint32_t pong_len = sizeof(t_pong_packet) - 1 + string_len;
  t_pong_packet *pong_packet = (t_pong_packet *)malloc(pong_len);
  pong_packet->server_time = millis();
  pong_packet->guid = 0;
  memcpy(&pong_packet->magic, packet_magic, sizeof(packet_magic));

  t_server_string *str = ParseString((uint8_t *)packet_string, strlen(packet_string));

  strcpy(str->port_ipv6, "");
  if(millis() < last_server_packet + 10000)
  {
      if(last_server_packet == 0)
      {
          sprintf(str->motd, "§oStatus: §6'%s:%d' §kcheck", ServerAddress, ServerPort);
      }
      else
      {
          sprintf(str->motd, "§oStatus: §2'%s:%d' online", ServerAddress, ServerPort);
      }
  }
  else
  {
      sprintf(str->motd, "§oStatus: §c'%s:%d' offline", ServerAddress, ServerPort);
  }
  char status[32];

  sprintf(status, ", RSSI: %i", WiFi.RSSI());
  strcat(str->sub_motd, status);

  /* now calc real server string length */
  string_len = AssembleString(str, &pong_packet->server_string);
  pong_packet->server_len = htons(string_len);

  int udp_length = 1 + sizeof(t_pong_packet) - 1 + string_len;
  uint8_t *udp_buffer = (uint8_t *)malloc(udp_length);

  udp_buffer[0] = PROT_UNCONN_PONG;
  memcpy(&udp_buffer[1], pong_packet, udp_length - 1);

  server_udp.writeTo((uint8_t*)udp_buffer, udp_length, dst, port);
  
  free(str);
  free(udp_buffer);
  free(pong_packet);
}


void ProxyServer::ClientReceived(AsyncUDPPacket& packet)
{
    uint8_t *payload = packet.data();
    size_t length = packet.length();
    
    ClientAddress = packet.remoteIP();
    ClientPort = packet.remotePort();
    
    if(payload[0] == PROT_UNCONN_PING)
    {
        if(current_config.verbose & 1)
        {
            Serial.printf("[Proxy] Received ping from client\n");
        }

        /* always also respond with the generic info entry */
        SendPacket(ClientAddress, ClientPort);
    }
    else if(payload[0] == PROT_UNCONN_PONG)
    {
        if(current_config.verbose & 1)
        {
            Serial.printf("[Proxy] Received pong from client\n");
        }
    }
    else
    {
        if(current_config.verbose & 1)
        {
            Serial.printf("[Proxy] Received packet from client\n");
        }
    }
    
    server_udp.writeTo(payload, length, proxy_server_addr, ServerPort);
    bytes_transferred += length;
}

void ProxyServer::ServerReceived(AsyncUDPPacket& packet)
{
    uint8_t *payload = packet.data();
    size_t length = packet.length();

    last_server_packet = millis();
    
    if(payload[0] == PROT_UNCONN_PING)
    {
        if(current_config.verbose & 1)
        {
            Serial.printf("[Proxy] Received ping from server - ");
        }
    }
    else if(payload[0] == PROT_UNCONN_PONG)
    {
        if(current_config.verbose & 1)
        {
          Serial.printf("[Proxy] Received pong from server\n");
        }
        t_pong_packet *pkt = (t_pong_packet *)malloc(length - 1 + 1024);
        memcpy(pkt, &payload[1], length - 1);
        t_server_string *str = ParseString(&pkt->server_string, ntohs(pkt->server_len));
        if(current_config.verbose & 1)
        {
          Serial.printf("    edition %s, game %s, motd %s, sub_motd %s, id %s, port4 %s, port6 %s\n", str->edition, str->game_type, str->motd, str->sub_motd, str->server_id, str->port_ipv4, str->port_ipv6);
        }

        sprintf(str->port_ipv4, "%d", LocalPort);
        strcpy(str->port_ipv6, "");
        strcpy(str->motd, str->sub_motd);
        strcat(str->motd, "§o§3 via proxy");
        strcpy(str->sub_motd, ServerName);
        
        int newlen = AssembleString(str, &pkt->server_string);
        pkt->server_len = htons(newlen);
        length = 1 + sizeof(t_pong_packet) - 1 + newlen;

        uint8_t *new_buffer = (uint8_t *)malloc(length);

        new_buffer[0] = PROT_UNCONN_PONG;
        memcpy(&new_buffer[1], pkt, length - 1);

        client_udp.writeTo(new_buffer, length, ClientAddress, ClientPort);
        
        free(str);
        free(pkt);
        free(new_buffer);
        return;
    }
    else
    {
        if(current_config.verbose & 1)
        {
            Serial.printf("[Proxy] Received packet from server\n");
        }
    }

    client_udp.writeTo(payload, length, ClientAddress, ClientPort);
    bytes_transferred += length;
}
