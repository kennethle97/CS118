#ifndef SERVER_H
#define SERVER_H


#include<iostream>
#include<cstring>
#include<cstdio>
#include<ctype.h>
#include<stdlib.h>
#include<fcntl.h>
#include<errno.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<unistd.h>
#include<netinet/in.h>
#include<map>
#include<regex>
#include<string>
#include<utility>
#include<sstream>
#include<ctime>

typedef std::pair<int, int> exclusion_range;
typedef std::pair<int, int> port_pair;
typedef std::pair<int, int> subnet_pair;


class Server {


    public: 
    
    Server(std::string config);
    struct IP_Packet;
    struct TCP_Packet;
    struct UDP_Packet;
    struct ip_port_addr;

    private:

    void parse_config(std::string config);
    uint32_t convert_ip_to_binary(const std::string& ip_address);
    bool check_excluded_ip_address(uint32_t source_ip,uint32_t dest_ip,uint16_t source_port,uint16_t dest_port);
    
    IP_Packet parse_IPv4_Header(char* packet);
    TCP_Packet get_tcp_packet(IP_Packet ip_header,char* packet);
    UDP_Packet get_udp_packet(IP_Packet ip_header,char* packet);

    bool valid_checksum(IP_Packet ip_header, char* packet);
    uint32_t calculate_checksum(void* data, size_t length,int option);
    char* deduct_TTL(char* packet);
    ip_port_addr get_ip_port_vals(char* buffer); 
    char* change_packet_vals(char* buffer,uint32_t source_ip,uint32_t dest_ip, uint16_t source_port, uint16_t dest_port);
    char* process_packet(char* packet);

    void run_server(const char* wan_ip);
    void establish_TCP_Connection(char* packet, uint32_t destIP, uint16_t destPort);
    void* client_sock_thread(void* client_sock);


    void printIPv4Header(IP_Packet& header);
    void print_tcp_packet(const TCP_Packet& tcp_packet);
    void print_udp_packet(const UDP_Packet& udp_packet);
    // void map_ip_port(std::string ip_address,port_pair pair_port );
    // void add_exclusion_range(ip_address_pair pair_ip,exclusion_range port_ranges);
    // void match_expression(std::string line);

    std::string wan_ip;
    std::string lan_ip;
    uint32_t wan_ip_bin;
    uint32_t lan_ip_bin;

    const char* wan_port_ip = "0.0.0.0";
    uint32_t lan_subnet_mask = 0xFFFFFF00;  // Default subnet mask for /24 subnet


    std::map<std::string, port_pair>  port_map;
    //ACL
    std::map<std::string,std::map<std::string,std::pair<exclusion_range,exclusion_range >>> exclusion_map;



};

#endif