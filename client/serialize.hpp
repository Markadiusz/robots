#ifndef __SERIALIZE_HPP
#define __SERIALIZE_HPP

#include "messages.hpp"

template <std::integral T> message_t serialize(T x);

message_t serialize(const std::string &x);

template <typename T> message_t serialize(const std::vector<T> &x);

template <typename K, typename V> message_t serialize(const std::map<K, V> &x);

message_t serialize(const Position &x);

message_t serialize(const Bomb &x);

message_t serialize(const Player &x);

message_t serialize(const Lobby &x);

message_t serialize(const Game &x);

message_t serialize(const DrawMessage &x);

message_t serialize(const Direction &x);

message_t serialize(const Join &x);

message_t serialize([[maybe_unused]] const PlaceBomb &x);

message_t serialize([[maybe_unused]] const PlaceBlock &x);

message_t serialize(const Move &x);

message_t serialize(const ClientMessage &x);

message_t serialize(const BombPlaced &x);

message_t serialize(const BombExploded &x);

message_t serialize(const PlayerMoved &x);

message_t serialize(const BlockPlaced &x);

message_t serialize(const Event &x);

message_t serialize(const Hello &x);

message_t serialize(const AcceptedPlayer &x);

message_t serialize(const GameStarted &x);

message_t serialize(const Turn &x);

message_t serialize(const GameEnded &x);

message_t serialize(const ServerMessage &x);

#endif // __SERIALIZE_HPP
