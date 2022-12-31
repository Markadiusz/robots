#ifndef __MESSAGES_HPP
#define __MESSAGES_HPP

#include <cassert>
#include <map>
#include <string>
#include <variant>
#include <vector>

using message_t = std::vector<uint8_t>;

using BombId = uint32_t;
using PlayerId = uint8_t;
using Score = uint32_t;

enum Direction { Up = 0, Right = 1, Down = 2, Left = 3 };

struct Position {
  uint16_t x;
  uint16_t y;
  auto operator<=>(const Position &) const = default;
  std::pair<int, int> move(const Direction &direction) const {
    switch (direction) {
    case Up:
      return {x, y + 1};
    case Right:
      return {x + 1, y};
    case Down:
      return {x, y - 1};
    case Left:
      return {x - 1, y};
    }
    assert(false);
  }
};

struct Bomb {
  Position position;
  uint16_t timer;
};

struct Player {
  std::string name;
  std::string address;
};

struct Lobby {
  std::string server_name;
  uint8_t players_count;
  uint16_t size_x;
  uint16_t size_y;
  uint16_t game_length;
  uint16_t explosion_radius;
  uint16_t bomb_timer;
  std::map<PlayerId, Player> players;
};

struct Game {
  std::string server_name;
  uint16_t size_x;
  uint16_t size_y;
  uint16_t game_length;
  uint16_t turn;
  std::map<PlayerId, Player> players;
  std::map<PlayerId, Position> player_positions;
  std::vector<Position> blocks;
  std::vector<Bomb> bombs;
  std::vector<Position> explosions;
  std::map<PlayerId, Score> scores;
};

struct DrawMessage {
  std::variant<Lobby, Game> m;
};

struct Join {
  std::string name;
};

struct PlaceBomb {};

struct PlaceBlock {};

struct Move {
  Direction direction;
};

struct ClientMessage {
  std::variant<Join, PlaceBomb, PlaceBlock, Move> m;
};

struct InputMessage {
  std::variant<PlaceBomb, PlaceBlock, Move> m;
};

struct BombPlaced {
  BombId id;
  Position position;
};

struct BombExploded {
  BombId id;
  std::vector<PlayerId> robots_destroyed;
  std::vector<Position> blocks_destroyed;
};

struct PlayerMoved {
  PlayerId id;
  Position position;
};

struct BlockPlaced {
  Position position;
};

struct Event {
  std::variant<BombPlaced, BombExploded, PlayerMoved, BlockPlaced> m;
};

struct Hello {
  std::string server_name;
  uint8_t players_count;
  uint16_t size_x;
  uint16_t size_y;
  uint16_t game_length;
  uint16_t explosion_radius;
  uint16_t bomb_timer;
};

struct AcceptedPlayer {
  PlayerId id;
  Player player;
};

struct GameStarted {
  std::map<PlayerId, Player> players;
};

struct Turn {
  uint16_t turn;
  std::vector<Event> events;
};

struct GameEnded {
  std::map<PlayerId, Score> scores;
};

struct ServerMessage {
  std::variant<Hello, AcceptedPlayer, GameStarted, Turn, GameEnded> m;
};

#endif // __MESSAGES_HPP
