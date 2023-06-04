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
    struct IPv4_Header;
    struct TCP_Header;
   

    private:

    void run_server(const char* wan_ip);
    void parse_config(std::string config);
    uint32_t convert_ip_to_binary(const std::string& ip_address);
    bool check_excluded_ip_address(uint32_t source_ip,uint32_t dest_ip,uint16_t source_port,uint16_t dest_port);
    
    IPv4_Header parse_IPv4_Header(const char* packet);
    TCP_Header parse_TCP_Header(uint8_t IPv4_header_length, const char* packet);
    void printIPv4Header(const IPv4_Header& header);
    // void map_ip_port(std::string ip_address,port_pair pair_port );
    // void add_exclusion_range(ip_address_pair pair_ip,exclusion_range port_ranges);
    // void match_expression(std::string line);

    const char* wan_ip;
    const char* lan_ip;
    const char* wan_port_ip = "0.0.0.0";

    std::map<std::string, port_pair>  port_map;
    //ACL
    std::map<std::string,std::map<std::string,std::pair<exclusion_range,exclusion_range >>> exclusion_map;



};

#endif