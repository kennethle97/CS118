#include "server.h"

#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 20
#define DEFAULT_PORT 8080


struct Server::IPv4_Header {
    uint16_t version_hlength_tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_and_fragment_offset;
    uint16_t time_to_live_and_protocol;
    uint16_t header_checksum;
    uint32_t source_ip;
    uint32_t destination_ip;
    uint8_t* options; 

};

struct Server::TCP_Packet {
    uint16_t source_port;
    uint16_t destination_port;
    uint32_t sequence_number;
    uint32_t acknowledgment_number;
    uint16_t data_offset_and_flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
    uint8_t* options_and_data;

};



Server::Server(std::string config) {
    parse_config(config);
    const char* packet = "\x45\x00\x00\x2a\x1e\x84\x40\x00\x40\x06\xae\x87\xc0\xa8\x01\x02\xac\x10\x00\x0a\x04\xd2\x00\x50\x00\x00\x00\x00\x50\x00\x00\x00\x00\x00\x00\x00\x3c\xfe\x00\x00\x00\x00\x00\x00";
    IPv4_Header parsed_header = parse_IPv4_Header(packet);
    TCP_Packet parsed_tcp_packet = parse_TCP_Packet(parsed_header,packet);
    printIPv4Header(parsed_header);
    printTCPHeader(parsed_tcp_packet);
    bool isvalid = valid_checksum(parsed_header,parsed_tcp_packet);
    std::cout<<isvalid<<'\n';
    // run_server();
}


Server::IPv4_Header Server::parse_IPv4_Header(const char* packet) {
    IPv4_Header header;

    const uint8_t* buffer = reinterpret_cast<const uint8_t*>(packet);
    header.version_hlength_tos = (buffer[0] << 8 )| buffer[1];
    header.total_length = (buffer[2] << 8) | buffer[3];
    header.identification = (buffer[4] << 8) | buffer[5];
    // header.flags_and_fragment_offset = (((header.flags_and_fragment_offset >> 8) & buffer[6]) << 8)| buffer[7];
    header.flags_and_fragment_offset = (buffer[6] << 8) | buffer[7];
    header.time_to_live_and_protocol = (buffer[8] << 8) | buffer[9];
    header.header_checksum = (buffer[10] << 8) | buffer[11];
    header.source_ip = (buffer[12] << 24) | (buffer[13] << 16) | (buffer[14] << 8) | buffer[15];
    header.destination_ip = (buffer[16] << 24) | (buffer[17] << 16) | (buffer[18] << 8) | buffer[19];
    //Need the options field as well to calculate the checksum for the IP packet.
    uint16_t header_length = (header.version_hlength_tos & 0x0F00) >> 8; 
    uint16_t options_length = (header_length - 5) * 4 ;
    const uint8_t* options_buffer = buffer + 20;
    if(options_length){
        memcpy(header.options,options_buffer,options_length);
        }
    else{
        header.options = nullptr;
    }
    return header;

}

Server::TCP_Packet Server::parse_TCP_Packet(IPv4_Header ip_header, const char* packet) {
    TCP_Packet tcp_packet;
    uint16_t ipv4_total_length = ip_header.total_length;
    uint8_t ipv4_header_length = (ip_header.version_hlength_tos & 0x0F00) >> 8;
    const uint8_t* buffer = reinterpret_cast<const uint8_t*>(packet);
    // Calculate the offset for the TCP header based on the header length
    int offset = ipv4_header_length * 4;
    const uint8_t* tcp_packet_buffer = buffer + offset;
    // memcpy(&tcp_packet, tcp_packet_buffer, sizeof(TCP_Packet));

    // Create a TCP_Packet struct and populate its fields from the buffer

    tcp_packet.source_port = (tcp_packet_buffer[0] << 8) | tcp_packet_buffer[1];
    tcp_packet.destination_port = (tcp_packet_buffer[2] << 8) | tcp_packet_buffer[3];
    tcp_packet.sequence_number = (tcp_packet_buffer[4] << 24) | (tcp_packet_buffer[5] << 16) | (tcp_packet_buffer[6] << 8) | tcp_packet_buffer[7];
    tcp_packet.acknowledgment_number = (tcp_packet_buffer[8] << 24) | (tcp_packet_buffer[9] << 16) | (tcp_packet_buffer[10] << 8) | tcp_packet_buffer[11];
    tcp_packet.data_offset_and_flags = (tcp_packet_buffer[12] << 8) | tcp_packet_buffer[13];
    tcp_packet.window_size = (tcp_packet_buffer[14] << 8) | tcp_packet_buffer[15];
    tcp_packet.checksum = (tcp_packet_buffer[16] << 8) | tcp_packet_buffer[17];
    tcp_packet.urgent_pointer = (tcp_packet_buffer[18] << 8) | tcp_packet_buffer[19];


    //Copy the rest of the data to calculate the checksum later.

    // int data_offset = (tcp_packet_buffer[12] & 0xF0 >> 4);
    uint16_t tcp_packet_length = ipv4_total_length - (ipv4_header_length * 4);
    const uint8_t* options_and_data_buffer = tcp_packet_buffer + 20;
    if(tcp_packet_length - 20 > 0){
        memcpy(tcp_packet.options_and_data, options_and_data_buffer, tcp_packet_length - 20);
    }
    else{
        tcp_packet.options_and_data = nullptr;
    }
    return tcp_packet;
}


bool Server::valid_checksum(IPv4_Header ip_header, TCP_Packet tcp_packet){

    uint8_t pseudo_header[12];
    // Copy the source IP address (32 bits) into the pseudo header
    memcpy(pseudo_header, &ip_header.source_ip, sizeof(uint32_t));
    // Copy the destination IP address (32 bits) into the pseudo header
    memcpy(pseudo_header + 4, &ip_header.destination_ip, sizeof(uint32_t));

    uint16_t res_and_protocol = static_cast<uint16_t>(ip_header.time_to_live_and_protocol & 0x00FF);
    memcpy(pseudo_header + 8, &res_and_protocol, sizeof(uint16_t));

    // Copy the TCP/UDP length (16 bits) into the pseudo header
    uint8_t header_length = (ip_header.version_hlength_tos & 0x0F00) >> 8;
    uint16_t tcp_length = static_cast<uint16_t>(ip_header.total_length - header_length * 4);
    memcpy(pseudo_header + 10, &tcp_length, sizeof(uint16_t));

    TCP_Packet tcp_copy = tcp_packet;
    //Set the value of the checksum to be 0 for the tcp copy of the packet.
    tcp_copy.checksum = 0;

    // uint16_t data_offset = (tcp_copy.data_offset_and_flags & 0xF000) >> 12;
    // // TCP header
    // uint16_t tcp_header_length = data_offset * 4;
    // uint16_t tcp_total_length = sizeof(tcp_packet); // Calculate the total length of the TCP segment
    uint32_t tcp_copy_sum = calculate_checksum(pseudo_header, sizeof(pseudo_header));
    if(tcp_copy.options_and_data == nullptr){
        tcp_copy_sum += calculate_checksum(&tcp_copy, sizeof(tcp_copy)- sizeof(tcp_copy.options_and_data) - 4);
    }
    else{
        tcp_copy_sum += calculate_checksum(&tcp_copy, sizeof(tcp_copy));
    }
     std::cout << "checkpoint4" << '\n';
    //If ip_copy_sum ends up being greater than 16 bits after adding two calll to calculate_checksum twice then we carry the bits over again.
    tcp_copy_sum = (tcp_copy_sum & 0xFFFF) + (tcp_copy_sum >> 16);
    
    while (tcp_copy_sum > 0xFFFF) {
        tcp_copy_sum = (tcp_copy_sum & 0xFFFF) + (tcp_copy_sum >> 16);
    }
    uint16_t tcp_checksum = tcp_copy_sum & 0xFFFF;
    //Cast it to a 16 bit unsigned int after the carry bits are added back in.
    tcp_checksum = ~tcp_checksum;

    //Calculate ip check sum, we can simply call calculate_checksum with the ipv4_header as our input.
    IPv4_Header ip_copy = ip_header;
    ip_copy.header_checksum = 0;

    uint32_t ip_copy_sum;
    if(ip_copy.options != nullptr){
        ip_copy_sum = calculate_checksum(&ip_copy,sizeof(ip_copy));
    }
    else{
        ip_copy_sum = calculate_checksum(&ip_copy,sizeof(ip_copy) - sizeof(ip_copy.options) - 4);
    }

    ip_copy_sum = (ip_copy_sum & 0xFFFF) + (ip_copy_sum >> 16);

    while (ip_copy_sum > 0xFFFF) {
        ip_copy_sum = (ip_copy_sum & 0xFFFF) + (ip_copy_sum >> 16);
    }
    uint16_t ip_checksum = ip_copy_sum & 0xFFFF;
    ip_checksum = ~ip_checksum;
    std::cout << "ip check_sum: " << ip_checksum << '\n';
    std::cout << "tcp check_sum: " << tcp_checksum << '\n';
    //If either of the checksum fails for tcp/ip layer then we return false.
    if(tcp_checksum != tcp_packet.checksum){
        std::cout<<"Tcp checksum failed"<<'\n';
    }
    if(ip_checksum != ip_header.header_checksum){
        std::cout<<"ip checksum failed"<<'\n';
    }


    if((tcp_checksum != tcp_packet.checksum) | (ip_checksum != ip_header.header_checksum)){
        return false;
    }
    else{
        return true;
    }
}

uint32_t Server::calculate_checksum(const void* data, size_t length) {
    const uint16_t* buffer = reinterpret_cast<const uint16_t*>(data);
    uint32_t sum = 0;

    for (size_t i = 0; i < length / 2; ++i) {
        std::cout<<"val: " << (buffer[i]) << "\niteration : " << i << '\n';
        sum += (buffer[i]);
        std::cout<<"sum: " << sum <<" iteration: " << i << '\n';
    }

    // If the length is odd, add the last byte
    if (length % 2 != 0) {
        sum += static_cast<uint16_t>(reinterpret_cast<const uint8_t*>(data)[length - 1]);
        std::cout<<"sum odd byte: " << sum <<" iteration: " << '\n';
    }

    return sum;
}


void Server::printIPv4Header(const IPv4_Header& header) {
    //used for testing parser.
    std::cout << "Version: " << static_cast<int>((header.version_hlength_tos & 0xF000 )>> 12) << '\n';
    std::cout << "Header Length: " << static_cast<int>((header.version_hlength_tos & 0x0F00)>>8) << '\n';
    std::cout << "Type of Service: " << static_cast<int>((header.version_hlength_tos) & 0x00FF) << '\n';
    std::cout << "Total Length: " << header.total_length << '\n';
    std::cout << "Identification: " << header.identification << '\n';
    std::cout << "Flags and Fragment Offset: " << header.flags_and_fragment_offset << '\n';
    std::cout << "Time to Live: " << static_cast<int>((header.time_to_live_and_protocol & 0xFF00) >> 8) << '\n';
    std::cout << "Protocol: " << static_cast<int>(header.time_to_live_and_protocol & 0x00FF) << '\n';
    std::cout << "Header Checksum: " << header.header_checksum << '\n';
    std::cout << "Source IP: " << header.source_ip << '\n';
    std::cout << "Destination IP: " << header.destination_ip << '\n';
    // std::cout << "Options :" << header.options << '\n';

    if (header.options!= nullptr) {
        std::cout << "Options and Data: " << header.options << std::endl;
    }
    else {
        std::cout << "Options and Data: <null>" << std::endl;
    }
}

void Server::printTCPHeader(const TCP_Packet& tcp_packet) {
    std::cout << "Source Port: " << tcp_packet.source_port << std::endl;
    std::cout << "Destination Port: " << tcp_packet.destination_port << std::endl;
    std::cout << "Sequence Number: " << tcp_packet.sequence_number << std::endl;
    std::cout << "Acknowledgment Number: " << tcp_packet.acknowledgment_number << std::endl;
    std::cout << "Data Offset and Flags: " << tcp_packet.data_offset_and_flags << std::endl;
    std::cout << "Window Size: " << tcp_packet.window_size << std::endl;
    std::cout << "Checksum: " << tcp_packet.checksum << std::endl;
    std::cout << "Urgent Pointer: " << tcp_packet.urgent_pointer << std::endl;
    // Assuming options_and_data is a null-terminated string
    if (tcp_packet.options_and_data != nullptr) {
        std::cout << "Options and Data: " << tcp_packet.options_and_data << std::endl;
    }
    else {
        std::cout << "Options and Data: <null>" << std::endl;
    }
}


uint32_t Server::convert_ip_to_binary(const std::string& ip_address) {
    std::stringstream ss(ip_address);
    std::string octet;
    unsigned int result = 0;

    while (std::getline(ss, octet, '.')) {
        unsigned int decimal_octet = std::stoi(octet);
        result = (result << 8) | decimal_octet;
    }

    return result;
}

void Server::parse_config(std::string config){

    std::cout << "Parsing Config" << std::endl;
    std::regex ip_regex(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}))");
    std::regex ip_port_regex(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}) (\d{1,5}) (\d{1,5}))");
    std::regex acl_regex(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/\d{1,2}) (\d{1,5})-(\d{1,5}) (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}/\d{1,2}) (\d+)-(\d+))");
    std::smatch match_regex;


    std::stringstream ss(config);
    std::string line;
    // First line is the router's LAN IP and the WAN IP
    std::getline(ss, line, '\n');
    size_t dwPos = line.find(' ');
    std::string LanIp = line.substr(0, dwPos);
    std::string WanIp = line.substr(dwPos + 1);

    std::cout << "Server's LAN IP: " << LanIp << std::endl
                << "Server's WAN IP: " << WanIp << std::endl;

    wan_ip = WanIp.c_str();
    lan_ip = LanIp.c_str();
    //Skip WAN 0.0.0.0 line as it will always be 0.0.0.0
    std::getline(ss, line, '\n');
    std::getline(ss, line, '\n');
    //Parse through the first part of the block until first \n character by checking if the line contains

    std::string::const_iterator search(line.cbegin());
    //Parse through each of the blocks until \n character by checking if the line contains the specified expression
    while (std::regex_search(search, line.cend(), match_regex, ip_regex)) {
        std::string ip_address = match_regex[1].str();
        if(port_map.find(ip_address) == port_map.end()){
            port_map[ip_address] = std::make_pair(0,0);
        }
        std::getline(ss, line, '\n');
        search = line.cbegin();
    }
    std::getline(ss, line, '\n');
    search = line.cbegin();
    while (std::regex_search(search, line.cend(), match_regex, ip_port_regex)) {
        std::string ip_address = match_regex[1].str();
        int lan_port = std::stoi(match_regex[2].str());
        int wan_port = std::stoi(match_regex[3].str());
        
        auto port_pair = std::make_pair(lan_port,wan_port);
        if(port_map.find(ip_address) == port_map.end()){
            std::cerr << "Error, specified port pairing for ip address not defined in LAN: " << ip_address <<std::endl;
            //return;
        }
        port_map[ip_address] = port_pair;
        std::getline(ss, line, '\n');
        search = line.cbegin();
    }
    std::getline(ss, line, '\n');
    search = line.cbegin();
    while (std::regex_search(search, line.cend(), match_regex, acl_regex)) {
        //Note that the the ip addresses for these blocks will have the form ip_address/subnet mask. Will need to seperate the the string later to check.
        std::string host_ip_address = match_regex[1].str();
        int host_start_port = std::stoi(match_regex[2].str());
        int host_end_port = std::stoi(match_regex[3].str());
        
        std::string client_ip_address = match_regex[4].str();
        int client_start_port = std::stoi(match_regex[5].str());
        int client_end_port = std::stoi(match_regex[6].str());

        auto host_port_range = std::make_pair(host_start_port,host_end_port);
        auto client_port_range = std::make_pair(client_start_port,client_end_port);

        if(exclusion_map.find(host_ip_address) != exclusion_map.end()){
            auto& map = exclusion_map[host_ip_address];
            if(map.find(client_ip_address) == map.end()){
                //makes sure that the same entry isnt being inputted twice.
                exclusion_map[host_ip_address][client_ip_address] = std::make_pair(host_port_range, client_port_range);
            }
        }
        else{
            exclusion_map[host_ip_address] = std::map<std::string, std::pair<exclusion_range, exclusion_range>>();
            exclusion_map[host_ip_address][client_ip_address] = std::make_pair(host_port_range, client_port_range);
        }   

        std::getline(ss, line, '\n');
        search = line.cbegin();
    }
    //output the port mappings of the LAN ip addresses to the WAN ip.
    for (auto it = port_map.begin(); it != port_map.end(); ++it) {
        std::cout << "LAN IP: " << it->first << "\nLAN Port: " << it->second.first << " WAN Port: " << it->second.second << std::endl;
    }
    //Output the entries of the exclusion map for testing
    std::cout << "Exclusion Map:" << std::endl;
    for (const auto& entry : exclusion_map) {
        std::cout << "Host IP: " << entry.first << std::endl;
        const auto& inner_map = entry.second;
        for (const auto& inner_entry : inner_map) {
            std::cout << "  Client IP: " << inner_entry.first << std::endl;
            const auto& exclusion_range_pair = inner_entry.second;
            std::cout << "    Excluded Host Port Ranges: " << exclusion_range_pair.first.first
                    << "-" << exclusion_range_pair.first.second << std::endl;
            std::cout << "    Excluded Client Port Ranges: " << exclusion_range_pair.second.first
                    << "-" << exclusion_range_pair.second.second << std::endl;
        }
    }
    std::cout<<std::endl;
}

bool Server::check_excluded_ip_address(uint32_t source_ip,uint32_t dest_ip,uint16_t source_port,uint16_t dest_port){
    int mask_length = 0;
    int host_ip = 0;
    int client_ip = 0;
    int temp_source_ip = 0;
    int temp_dest_ip = 0;

    // std::cout << "source_ip " << source_ip << std::endl;
    // std::cout << "dest_ip " << dest_ip << std::endl;
    for(const auto& entry : exclusion_map){
        //Takes the a string with the format "192.168.1.0/24" and converts the first part ip to binary. Takes the last part for the mask." 
        host_ip  = convert_ip_to_binary(entry.first.substr(0,entry.first.find_last_of('/')));
        mask_length = std::stoi(entry.first.substr(entry.first.find_last_of('/') + 1));
        // std::cout << "host_ip " << host_ip << std::endl;
        // std::cout << "mask_length " << mask_length << std::endl;
        //Get the number of bits to actually shift 
        mask_length = 32 - mask_length;
        // std::cout << "mask_length " << mask_length << std::endl;
        //We shift right and left the length of the bit mask for comparison. Then we perform bitwise & with host_ip to find a match.
        temp_source_ip = source_ip >> mask_length << mask_length;
        temp_source_ip = host_ip & temp_source_ip;
        // std::cout << "temp_source_ip " << temp_source_ip <<std::endl;

        if(temp_source_ip == host_ip){

            const auto& inner_map = entry.second;
            for(const auto& inner_entry : inner_map){
                client_ip  = convert_ip_to_binary(inner_entry.first.substr(0,inner_entry.first.find_last_of('/')));
                mask_length = std::stoi(inner_entry.first.substr(inner_entry.first.find_last_of('/') + 1));
                //Get the number of bits to actually shift 
                mask_length = 32 - mask_length;
                // std::cout << "mask length " << mask_length << std::endl;

                //We shift right and left the length of the bit mask for comparison. Then we perform bitwise & with host_ip to find a match.
                temp_dest_ip = dest_ip >> mask_length << mask_length;
                temp_dest_ip = client_ip & temp_dest_ip;
                // std::cout << "temp_dest_ip " << temp_dest_ip <<std::endl;
                // std::cout << "client_ip " << client_ip <<std::endl;
                if(temp_dest_ip == client_ip){

                    const auto& exclusion_range_pair = inner_entry.second;
                    int host_range_lower = exclusion_range_pair.first.first;
                    int host_range_upper = exclusion_range_pair.first.second;
                    int client_range_lower = exclusion_range_pair.second.first;
                    int client_range_upper = exclusion_range_pair.second.second;

                    // std::cout << "    Excluded Host Port Ranges: " << exclusion_range_pair.first.first
                    // << "-" << exclusion_range_pair.first.second << std::endl;
                    // std::cout << "    Excluded Client Port Ranges: " << exclusion_range_pair.second.first
                    // << "-" << exclusion_range_pair.second.second << std::endl;

                    if(host_range_lower <= source_port && source_port <= host_range_upper 
                        && client_range_lower <= dest_port && dest_port <= client_range_upper){
                            //Returns true if the ip address pair and port numbers are excluded from eachother.
                            return true;
                        }
                    }
                }
            }
        }
    return false;
}

// void Server::run_server(const char* wan_ip);{

//     pthread_t threads[MAX_CONNECTIONS];


//     int wan_fd;
//     int new_client_socket;

//     //Initialize array of sockets to be 0s
//     client_sockets[MAX_CONNECTIONS];
//     memset(client_sockets, 0, sizeof(client_socket));

//     struct sockaddr_in server_addr;
//     struct sockaddr_in client_addr;
//     socklen_t client_addr_size = sizeof(client_addr);
//     char buffer[BUFFER_SIZE];

//     if((wan_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1){
//         perror("socket");
//         exit(1);
//     }
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(DEFAULT_PORT);
//     server_addr.sin_addr.s_addr = inet_addr(wan_ip);

//     memset(server_addr.sin_zero, '\0',sizeof(server_addr.sin_zero));

//     /*Binding the socket*/
//     printf("Binding socket to %d\n",port_number);

//     if(bind(wan_fd, (struct sockaddr*) &server_addr, 
//         sizeof(server_addr)) == -1){
//             perror("Error in binding the socket");
//             exit(1);
//         }

//     /*Fixes error when port number is in use.*/
//     int yes=1;

//     if(setsockopt(wan_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1){
//         perror("Cannot reset socket options on wan_fd");
//         exit(1);
//     }

//     if(listen(wan_fd, BACKLOG) == -1){
//         perror("Error in listening to socket");
//         exit(1);
//         }

//      printf("Listening to socket %d\n",port_number);

//      printf("Waiting to accept connection from client.\n");

//     while(1){

//         //Accepting client connections to server 
//         new_client_socket = accept(wan_fd, (struct sockaddr *) &client_addr,&client_addr_size); 
//         if(client_fd == -1){
//             perror("Error in accepting client connection");
//             exit(1);
//         }
//         for (int i = 0; i < MAXCLIENTS; i++) {
//                 if (client_sockets[i] == 0) {
//                     client_sockets[i] = new_client_socket;
//                     break;
//                 }
//         }
//         //getting client_ip address and port number and printing it to console.
//         char client_ip[INET_ADDRSTRLEN];
//         inet_ntop(AF_INET, &client_addr.sin_addr, client_ip,sizeof(client_ip));
//         int client_port = ntohs(client_addr.sin_port);
//         printf("Accepted connection from client:\n %s:%d\n", client_ip, client_port);


//         //Now to read HTTP request from client

//         memset(buffer, 0, BUFFER_SIZE);
//         recv(client_fd, buffer, BUFFER_SIZE, 0);
//         printf("Request Received:\n%s\n",buffer);
//     }
// }

// void client_sock_thread(void* client_sock){
//     int client_socket = *((int*)client_sock);
//     char buffer[BUFFER_SIZE];
//     int valread;

//     while(1){
//         // Receive data from client if there is data
//         valread = read(client_sock, buffer, BUFFER_SIZE);
//         buffer[valread] = '\0';

//         if (valread == 0) {
//             close(client_sock);
//             // Update client_sockets array within a critical section if needed
//             // ...
//         } else {
//             printf("Client %d: %s\n", client_sock, buffer);
//             send(client_sock, buffer, strlen(buffer), 0);
//         }
//     }
//         return NULL;
// }