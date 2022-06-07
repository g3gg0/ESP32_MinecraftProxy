#ifndef __PROXY_SERVER__
#define __PROXY_SERVER__

#define PROT_UNCONN_PING 0x01
#define PROT_UNCONN_PONG 0x1C
#define STR_FIELD_SIZE 64
#define STR_FIELD_COUNT 12

/* this type has STR_FIELD_COUNT uniform entries for simpler string parsing */
typedef struct {
    char edition[STR_FIELD_SIZE];
    char motd[STR_FIELD_SIZE];
    char prot_ver[STR_FIELD_SIZE];
    char game_ver[STR_FIELD_SIZE];
    char players[STR_FIELD_SIZE];
    char max_players[STR_FIELD_SIZE];
    char server_id[STR_FIELD_SIZE];
    char sub_motd[STR_FIELD_SIZE];
    char game_type[STR_FIELD_SIZE];
    char extra[STR_FIELD_SIZE];
    char port_ipv4[STR_FIELD_SIZE];
    char port_ipv6[STR_FIELD_SIZE];
} t_server_string;

typedef struct {
    uint64_t server_time;
    uint64_t guid;
    uint8_t magic[16];
    uint16_t server_len;
    uint8_t server_string;
} t_pong_packet;

class ProxyServer
{
  public:
    ProxyServer (char *, char *, int, int);
    void ClientReceived(AsyncUDPPacket&);
    void ServerReceived(AsyncUDPPacket&);
    void Loop();
    char *ServerName;
    char *ServerAddress;
    int ServerPort;
    int LocalPort;
    
    int ClientPort;
    IPAddress ClientAddress;
        
  private:
    AsyncUDP client_udp;
    AsyncUDP server_udp;
    IPAddress proxy_client_addr;
    IPAddress proxy_server_addr;
    int proxy_out_port;
    uint64_t last_server_packet = 0;
    char *server_name;

    int AssembleString(t_server_string *server_str, uint8_t *buf);
    t_server_string *ParseString(uint8_t *string, int length);
    void SendPacket(IPAddress dst, uint16_t port);
};


#endif
