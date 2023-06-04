

#include "server.h"
int main() {

  std::ostringstream oss;
  std::string line;

    // Read the configuration from stdin
  while (std::getline(std::cin, line)) {
      oss << line << '\n';
  }

  std::string config_data = oss.str();

  if(config_data.empty()){
      std::cerr << "Error: Empty configuration" << std::endl;

  }


  // Process the config_data_cstr as needed
  std::cout << "Configuration: " << config_data << std::endl;

  Server server(config_data);
  // server.parse_config(config_data);
  return 0;

}
