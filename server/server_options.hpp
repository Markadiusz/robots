#ifndef __SERVER_OPTIONS_HPP
#define __SERVER_OPTIONS_HPP

#include <string>

struct ServerOptions {
  uint16_t bomb_timer;
  uint16_t players_count;
  uint64_t turn_duration;
  uint16_t explosion_radius;
  uint16_t initial_blocks;
  uint16_t game_length;
  std::string server_name;
  uint16_t port;
  uint32_t seed;
  uint16_t size_x;
  uint16_t size_y;
};

ServerOptions get_server_options(int argc, char *argv[]);

#endif // __SERVER_OPTIONS_HPP
