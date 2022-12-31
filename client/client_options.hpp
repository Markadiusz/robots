#ifndef __CLIENT_OPTIONS_HPP
#define __CLIENT_OPTIONS_HPP

#include <string>

struct ClientOptions {
  std::string gui_address;
  uint16_t gui_port;
  std::string player_name;
  uint16_t port;
  std::string server_address;
  uint16_t server_port;
};

ClientOptions get_client_options(int argc, char **argv);

#endif // __CLIENT_OPTIONS_HPP
