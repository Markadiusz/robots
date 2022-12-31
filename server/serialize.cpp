#include "serialize.hpp"

template <typename T> void append(std::vector<T> &x, const std::vector<T> &y) {
  x.insert(x.end(), y.begin(), y.end());
}

template <std::integral T> message_t serialize(T x) {
  static constexpr int mod = (1 << 8);
  message_t ret;
  for (size_t i = 0; i < sizeof(T); ++i) {
    ret.emplace_back(x % mod);
    x = T(x / mod);
  }
  reverse(ret.begin(), ret.end());
  return ret;
}

message_t serialize(const std::string &x) {
  message_t ret{uint8_t(x.length())};
  ret.insert(ret.end(), x.begin(), x.end());
  return ret;
}

template <typename T> message_t serialize(const std::vector<T> &x) {
  message_t ret{serialize(uint32_t(x.size()))};
  for (const auto &v : x) {
    append(ret, serialize(v));
  }
  return ret;
}

template <typename K, typename V> message_t serialize(const std::map<K, V> &x) {
  message_t ret{serialize(uint32_t(x.size()))};
  for (const auto &[k, v] : x) {
    append(ret, serialize(k));
    append(ret, serialize(v));
  }
  return ret;
}

message_t serialize(const Position &x) {
  message_t ret{serialize(x.x)};
  append(ret, serialize(x.y));
  return ret;
}

message_t serialize(const Bomb &x) {
  message_t ret{serialize(x.position)};
  append(ret, serialize(x.timer));
  return ret;
}

message_t serialize(const Player &x) {
  message_t ret{serialize(x.name)};
  append(ret, serialize(x.address));
  return ret;
}

message_t serialize(const Lobby &x) {
  message_t ret{0};
  append(ret, serialize(x.server_name));
  append(ret, serialize(x.players_count));
  append(ret, serialize(x.size_x));
  append(ret, serialize(x.size_y));
  append(ret, serialize(x.game_length));
  append(ret, serialize(x.explosion_radius));
  append(ret, serialize(x.bomb_timer));
  append(ret, serialize(x.players));
  return ret;
}

message_t serialize(const Game &x) {
  message_t ret{1};
  append(ret, serialize(x.server_name));
  append(ret, serialize(x.size_x));
  append(ret, serialize(x.size_y));
  append(ret, serialize(x.game_length));
  append(ret, serialize(x.turn));
  append(ret, serialize(x.players));
  append(ret, serialize(x.player_positions));
  append(ret, serialize(x.blocks));
  append(ret, serialize(x.bombs));
  append(ret, serialize(x.explosions));
  append(ret, serialize(x.scores));
  return ret;
}

message_t serialize(const DrawMessage &x) {
  if (std::holds_alternative<Lobby>(x.m))
    return serialize(std::get<Lobby>(x.m));
  else
    return serialize(std::get<Game>(x.m));
}

message_t serialize(const Direction &x) { return serialize(uint8_t(x)); }

message_t serialize(const Join &x) {
  message_t ret{0};
  append(ret, serialize(x.name));
  return ret;
}

message_t serialize([[maybe_unused]] const PlaceBomb &x) { return {1}; }

message_t serialize([[maybe_unused]] const PlaceBlock &x) { return {2}; }

message_t serialize(const Move &x) {
  message_t ret{3};
  append(ret, serialize(x.direction));
  return ret;
}

message_t serialize(const ClientMessage &x) {
  if (std::holds_alternative<Join>(x.m))
    return serialize(std::get<Join>(x.m));
  else if (std::holds_alternative<PlaceBomb>(x.m))
    return serialize(std::get<PlaceBomb>(x.m));
  else if (std::holds_alternative<PlaceBlock>(x.m))
    return serialize(std::get<PlaceBlock>(x.m));
  else
    return serialize(std::get<Move>(x.m));
}

message_t serialize(const BombPlaced &x) {
  message_t ret{0};
  append(ret, serialize(x.id));
  append(ret, serialize(x.position));
  return ret;
}

message_t serialize(const BombExploded &x) {
  message_t ret{1};
  append(ret, serialize(x.id));
  append(ret, serialize(x.robots_destroyed));
  append(ret, serialize(x.blocks_destroyed));
  return ret;
}

message_t serialize(const PlayerMoved &x) {
  message_t ret{2};
  append(ret, serialize(x.id));
  append(ret, serialize(x.position));
  return ret;
}

message_t serialize(const BlockPlaced &x) {
  message_t ret{3};
  append(ret, serialize(x.position));
  return ret;
}

message_t serialize(const Event &x) {
  if (std::holds_alternative<BombPlaced>(x.m))
    return serialize(std::get<BombPlaced>(x.m));
  else if (std::holds_alternative<BombExploded>(x.m))
    return serialize(std::get<BombExploded>(x.m));
  else if (std::holds_alternative<PlayerMoved>(x.m))
    return serialize(std::get<PlayerMoved>(x.m));
  else
    return serialize(std::get<BlockPlaced>(x.m));
}

message_t serialize(const Hello &x) {
  message_t ret{0};
  append(ret, serialize(x.server_name));
  append(ret, serialize(x.players_count));
  append(ret, serialize(x.size_x));
  append(ret, serialize(x.size_y));
  append(ret, serialize(x.game_length));
  append(ret, serialize(x.explosion_radius));
  append(ret, serialize(x.bomb_timer));
  return ret;
}

message_t serialize(const AcceptedPlayer &x) {
  message_t ret{1};
  append(ret, serialize(x.id));
  append(ret, serialize(x.player));
  return ret;
}

message_t serialize(const GameStarted &x) {
  message_t ret{2};
  append(ret, serialize(x.players));
  return ret;
}

message_t serialize(const Turn &x) {
  message_t ret{3};
  append(ret, serialize(x.turn));
  append(ret, serialize(x.events));
  return ret;
}

message_t serialize(const GameEnded &x) {
  message_t ret{4};
  append(ret, serialize(x.scores));
  return ret;
}

message_t serialize(const ServerMessage &x) {
  if (std::holds_alternative<Hello>(x.m))
    return serialize(std::get<Hello>(x.m));
  else if (std::holds_alternative<AcceptedPlayer>(x.m))
    return serialize(std::get<AcceptedPlayer>(x.m));
  else if (std::holds_alternative<GameStarted>(x.m))
    return serialize(std::get<GameStarted>(x.m));
  else if (std::holds_alternative<Turn>(x.m))
    return serialize(std::get<Turn>(x.m));
  else
    return serialize(std::get<GameEnded>(x.m));
}
