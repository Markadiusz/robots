#ifndef __DESERIALIZE_HPP
#define __DESERIALIZE_HPP

#include "messages.hpp"
#include "tcp_reader.hpp"

struct CouldNotDeserialize : public std::exception {
  const char *what() const throw() { return "CouldNotDeserialize"; }
};

// TCP

template <typename T> T deserialize(TCPReader &tcp_reader);

std::string deserialize_string(TCPReader &tcp_reader);

template <typename T> std::vector<T> deserialize_vector(TCPReader &tcp_reader);

template <typename K, typename V>
std::map<K, V> deserialize_map(TCPReader &tcp_reader);

Position deserialize_position(TCPReader &tcp_reader);

Bomb deserialize_bomb(TCPReader &tcp_reader);

Player deserialize_player(TCPReader &tcp_reader);

Join deserialize_join(TCPReader &tcp_reader);

ClientMessage deserialize_client_message(TCPReader &tcp_reader);

BombPlaced deserialize_bomb_placed(TCPReader &tcp_reader);

BombExploded deserialize_bomb_exploded(TCPReader &tcp_reader);

PlayerMoved deserialize_player_moved(TCPReader &tcp_reader);

BlockPlaced deserialize_block_placed(TCPReader &tcp_reader);

Event deserialize_event(TCPReader &tcp_reader);

Hello deserialize_hello(TCPReader &tcp_reader);

AcceptedPlayer deserialize_accepted_player(TCPReader &tcp_reader);

GameStarted deserialize_game_started(TCPReader &tcp_reader);

Turn deserialize_turn(TCPReader &tcp_reader);

GameEnded deserialize_game_ended(TCPReader &tcp_reader);

ServerMessage deserialize_server_message(TCPReader &tcp_reader);

// UDP

Direction deserialize_direction(const message_t &message, size_t &pos);

Move deserialize_move(const message_t &message, size_t &pos);

InputMessage deserialize_input_message(const message_t &message, size_t &pos);

#endif // __DESERIALIZE_HPP
