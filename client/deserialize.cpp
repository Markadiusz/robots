#include "deserialize.hpp"

template <typename T> T deserialize(TCPReader &tcp_reader) {
  if constexpr (std::integral<T>) {
    return deserialize_integral<T>(tcp_reader);
  } else if constexpr (std::is_same<Position, T>()) {
    return deserialize_position(tcp_reader);
  } else if constexpr (std::is_same<Bomb, T>()) {
    return deserialize_bomb(tcp_reader);
  } else if constexpr (std::is_same<Event, T>()) {
    return deserialize_event(tcp_reader);
  } else if constexpr (std::is_same<Player, T>()) {
    return deserialize_player(tcp_reader);
  }
}

template <std::integral T> T deserialize_integral(TCPReader &tcp_reader) {
  static constexpr int mod = (1 << 8);
  message_t message;
  try {
    message = tcp_reader.read(sizeof(T));
  } catch (const InvalidTCPMessage &) {
    throw CouldNotDeserialize();
  }
  T ret = 0;
  for (size_t i = 0; i < sizeof(T); ++i) {
    ret = T(ret * mod + message[i]);
  }
  return ret;
}

std::string deserialize_string(TCPReader &tcp_reader) {
  auto len = deserialize<uint8_t>(tcp_reader);
  message_t message;
  try {
    message = tcp_reader.read(len);
  } catch (const InvalidTCPMessage &) {
    throw CouldNotDeserialize();
  }
  return std::string(message.begin(), message.end());
}

template <typename T> std::vector<T> deserialize_vector(TCPReader &tcp_reader) {
  auto len = deserialize<uint32_t>(tcp_reader);
  std::vector<T> ret(len);
  for (uint32_t i = 0; i < len; ++i) {
    ret[i] = deserialize<T>(tcp_reader);
  }
  return ret;
}

template <typename K, typename V>
std::map<K, V> deserialize_map(TCPReader &tcp_reader) {
  auto len = deserialize<uint32_t>(tcp_reader);
  std::map<K, V> ret;
  for (uint32_t i = 0; i < len; ++i) {
    auto key = deserialize<K>(tcp_reader);
    auto value = deserialize<V>(tcp_reader);
    ret.emplace(key, value);
  }
  return ret;
}

Position deserialize_position(TCPReader &tcp_reader) {
  Position ret;
  ret.x = deserialize<uint16_t>(tcp_reader);
  ret.y = deserialize<uint16_t>(tcp_reader);
  return ret;
}

Bomb deserialize_bomb(TCPReader &tcp_reader) {
  Bomb ret;
  ret.position = deserialize_position(tcp_reader);
  ret.timer = deserialize<uint16_t>(tcp_reader);
  return ret;
}

Player deserialize_player(TCPReader &tcp_reader) {
  Player ret;
  ret.name = deserialize_string(tcp_reader);
  ret.address = deserialize_string(tcp_reader);
  return ret;
}

Join deserialize_join(TCPReader &tcp_reader) {
  Join ret;
  ret.name = deserialize_string(tcp_reader);
  return ret;
}

Direction deserialize_direction(TCPReader &tcp_reader) {
  auto type = deserialize<uint8_t>(tcp_reader);
  if (type > 3) {
    throw CouldNotDeserialize();
  }
  return Direction(type);
}

Move deserialize_move(TCPReader &tcp_reader) {
  Move ret;
  ret.direction = deserialize_direction(tcp_reader);
  return ret;
}

ClientMessage deserialize_client_message(TCPReader &tcp_reader) {
  auto type = deserialize<uint8_t>(tcp_reader);
  if (type == 0) {
    return ClientMessage{deserialize_join(tcp_reader)};
  } else if (type == 1) {
    return ClientMessage{PlaceBomb{}};
  } else if (type == 2) {
    return ClientMessage{PlaceBlock{}};
  } else if (type == 3) {
    return ClientMessage{deserialize_move(tcp_reader)};
  } else {
    throw CouldNotDeserialize();
  }
}

BombPlaced deserialize_bomb_placed(TCPReader &tcp_reader) {
  BombPlaced ret;
  ret.id = deserialize<BombId>(tcp_reader);
  ret.position = deserialize_position(tcp_reader);
  return ret;
}

BombExploded deserialize_bomb_exploded(TCPReader &tcp_reader) {
  BombExploded ret;
  ret.id = deserialize<BombId>(tcp_reader);
  ret.robots_destroyed = deserialize_vector<PlayerId>(tcp_reader);
  ret.blocks_destroyed = deserialize_vector<Position>(tcp_reader);
  return ret;
}

PlayerMoved deserialize_player_moved(TCPReader &tcp_reader) {
  PlayerMoved ret;
  ret.id = deserialize<PlayerId>(tcp_reader);
  ret.position = deserialize_position(tcp_reader);
  return ret;
}

BlockPlaced deserialize_block_placed(TCPReader &tcp_reader) {
  BlockPlaced ret;
  ret.position = deserialize_position(tcp_reader);
  return ret;
}

Event deserialize_event(TCPReader &tcp_reader) {
  auto type = deserialize<uint8_t>(tcp_reader);
  if (type == 0) {
    return Event{deserialize_bomb_placed(tcp_reader)};
  } else if (type == 1) {
    return Event{deserialize_bomb_exploded(tcp_reader)};
  } else if (type == 2) {
    return Event{deserialize_player_moved(tcp_reader)};
  } else if (type == 3) {
    return Event{deserialize_block_placed(tcp_reader)};
  } else {
    throw CouldNotDeserialize();
  }
}

Hello deserialize_hello(TCPReader &tcp_reader) {
  Hello ret;
  ret.server_name = deserialize_string(tcp_reader);
  ret.players_count = deserialize<uint8_t>(tcp_reader);
  ret.size_x = deserialize<uint16_t>(tcp_reader);
  ret.size_y = deserialize<uint16_t>(tcp_reader);
  ret.game_length = deserialize<uint16_t>(tcp_reader);
  ret.explosion_radius = deserialize<uint16_t>(tcp_reader);
  ret.bomb_timer = deserialize<uint16_t>(tcp_reader);
  return ret;
}

AcceptedPlayer deserialize_accepted_player(TCPReader &tcp_reader) {
  AcceptedPlayer ret;
  ret.id = deserialize<PlayerId>(tcp_reader);
  ret.player = deserialize_player(tcp_reader);
  return ret;
}

GameStarted deserialize_game_started(TCPReader &tcp_reader) {
  GameStarted ret;
  ret.players = deserialize_map<PlayerId, Player>(tcp_reader);
  return ret;
}

Turn deserialize_turn(TCPReader &tcp_reader) {
  Turn ret;
  ret.turn = deserialize<uint16_t>(tcp_reader);
  ret.events = deserialize_vector<Event>(tcp_reader);
  return ret;
}

GameEnded deserialize_game_ended(TCPReader &tcp_reader) {
  GameEnded ret;
  ret.scores = deserialize_map<PlayerId, Score>(tcp_reader);
  return ret;
}

ServerMessage deserialize_server_message(TCPReader &tcp_reader) {
  auto type = deserialize<uint8_t>(tcp_reader);
  if (type == 0) {
    return ServerMessage{deserialize_hello(tcp_reader)};
  } else if (type == 1) {
    return ServerMessage{deserialize_accepted_player(tcp_reader)};
  } else if (type == 2) {
    return ServerMessage{deserialize_game_started(tcp_reader)};
  } else if (type == 3) {
    return ServerMessage{deserialize_turn(tcp_reader)};
  } else if (type == 4) {
    return ServerMessage{deserialize_game_ended(tcp_reader)};
  } else {
    throw CouldNotDeserialize();
  }
}

uint8_t deserialize_u8(const message_t &message, size_t &pos) {
  if (pos >= message.size())
    throw CouldNotDeserialize();
  return message[pos++];
}

Direction deserialize_direction(const message_t &message, size_t &pos) {
  auto type = deserialize_u8(message, pos);
  if (type > 3) {
    throw CouldNotDeserialize();
  }
  return Direction(type);
}

Move deserialize_move(const message_t &message, size_t &pos) {
  Move ret;
  ret.direction = deserialize_direction(message, pos);
  return ret;
}

InputMessage deserialize_input_message(const message_t &message, size_t &pos) {
  auto type = deserialize_u8(message, pos);
  if (type == 0) {
    return InputMessage{PlaceBomb{}};
  } else if (type == 1) {
    return InputMessage{PlaceBlock{}};
  } else if (type == 2) {
    return InputMessage{deserialize_move(message, pos)};
  } else {
    throw CouldNotDeserialize();
  }
}
