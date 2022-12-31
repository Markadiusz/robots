#include <array>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <random>
#include <set>
#include <shared_mutex>

#include "deserialize.hpp"
#include "messages.hpp"
#include "serialize.hpp"
#include "server_options.hpp"

using boost::asio::ip::tcp;
using socket_t = std::shared_ptr<tcp::socket>;
using Lock = std::lock_guard<std::mutex>;
using WLock = std::shared_lock<std::shared_mutex>;
using RLock = std::unique_lock<std::shared_mutex>;

std::set<socket_t> clients;
std::mutex clients_mutex;
std::map<socket_t, ClientMessage> client_messages;
std::mutex client_messages_mutex;
// stops the main thread so that new clients can relay previous server messages
std::shared_mutex catching_up_mutex;

// previous server messages
Hello hello;
std::vector<AcceptedPlayer> accepted_players;
GameStarted game_started;
std::vector<Turn> turns;

bool is_lobby = true;

template <typename T> void send(socket_t socket, const T &message) {
  try {
    boost::asio::write(*socket,
                       boost::asio::buffer(serialize(ServerMessage{message})));
  } catch (...) {
  }
}

// assumes that the caller acquired the clients_mutex
template <typename T> void send_to_all_clients(const T &message) {
  for (const auto &client : clients) {
    send(client, message);
  }
}

void handle_connection(socket_t socket) {
  // sending previous server messages
  {
    WLock w_lock(catching_up_mutex);
    send(socket, hello);
    if (is_lobby) {
      for (const auto &accepted_player : accepted_players)
        send(socket, accepted_player);
    } else {
      send(socket, game_started);
      for (const auto &turn : turns)
        send(socket, turn);
    }
    Lock lock(clients_mutex);
    clients.emplace(socket);
  }
  // listening for client messages
  TCPReader tcp_reader(*socket);
  for (;;) {
    try {
      auto client_message = deserialize_client_message(tcp_reader);
      Lock lock(client_messages_mutex);
      client_messages[socket] = client_message;
    } catch (...) {
      Lock lock(clients_mutex);
      clients.erase(socket);
      return;
    }
  }
}

void init_hello(const ServerOptions &server_options) {
  hello.server_name = server_options.server_name;
  hello.players_count = uint8_t(server_options.players_count);
  hello.size_x = server_options.size_x;
  hello.size_y = server_options.size_y;
  hello.game_length = server_options.game_length;
  hello.explosion_radius = server_options.explosion_radius;
  hello.bomb_timer = server_options.bomb_timer;
}

int main(int argc, char **argv) {
  auto server_options = get_server_options(argc, argv);
  init_hello(server_options);

  boost::asio::io_context io_context;
  std::unique_ptr<tcp::acceptor> acceptor;
  try {
    acceptor = std::make_unique<tcp::acceptor>(tcp::acceptor(
        io_context, tcp::endpoint(tcp::v6(), server_options.port)));
  } catch (...) {
    std::cerr << "Could not bind to the given port" << std::endl;
    exit(1);
  }
  std::thread acceptor_thread{[&] {
    for (;;) {
      auto socket = std::make_shared<tcp::socket>(tcp::socket(io_context));
      acceptor->accept(*socket);
      socket->set_option(tcp::no_delay(true));
      std::thread handle_connection_thread{handle_connection,
                                           std::move(socket)};
      handle_connection_thread.detach();
    }
  }};

  std::minstd_rand random(server_options.seed);
  auto generate_position = [&] {
    Position position;
    position.x = uint16_t(random() % server_options.size_x);
    position.y = uint16_t(random() % server_options.size_y);
    return position;
  };
  auto is_legal = [&](int x, int y) {
    return x >= 0 && x < server_options.size_x && y >= 0 &&
           y < server_options.size_y;
  };

  for (;;) {
    // gathering players
    std::map<PlayerId, socket_t> player_to_socket;
    std::set<socket_t> playing_clients;
    std::map<PlayerId, Player> players;
    auto has_all_players = [&] {
      if (int(playing_clients.size()) == int(server_options.players_count)) {
        // sending GameStarted
        RLock r_lock(catching_up_mutex);
        Lock lock(clients_mutex);
        game_started.players = players;
        send_to_all_clients(game_started);
        is_lobby = false;
        return true;
      }
      return false;
    };
    has_all_players();
    while (is_lobby) {
      Lock lock(client_messages_mutex);
      for (const auto &[client, client_message] : client_messages) {
        if (playing_clients.contains(client))
          continue;
        if (!std::holds_alternative<Join>(client_message.m))
          continue;

        PlayerId player_id = PlayerId(playing_clients.size());
        Player player;
        player.name = get<Join>(client_message.m).name;
        try {
          player.address =
              boost::lexical_cast<std::string>(client->remote_endpoint());
        } catch (...) {
          continue;
        }
        players[player_id] = player;
        AcceptedPlayer accepted_player;
        accepted_player.id = player_id;
        accepted_player.player = player;
        playing_clients.emplace(client);
        player_to_socket[player_id] = client;
        // sending AcceptedPlayer
        {
          RLock r_lock(catching_up_mutex);
          Lock lock(clients_mutex);
          send_to_all_clients(accepted_player);
          accepted_players.emplace_back(accepted_player);
        }

        if (has_all_players())
          break;
      }
      client_messages.clear();
    }

    std::map<PlayerId, Position> player_positions;
    std::set<Position> blocks;
    std::map<PlayerId, Score> scores;
    std::map<BombId, Bomb> ticking_bombs;
    BombId next_bomb_id = 0;

    auto generate_turn_0 = [&] {
      Turn turn_0;
      turn_0.turn = 0;
      for (PlayerId player_id = 0; player_id < server_options.players_count;
           ++player_id) {
        auto position = generate_position();
        player_positions[player_id] = position;
        scores[player_id] = 0;
        PlayerMoved player_moved;
        player_moved.id = player_id;
        player_moved.position = position;
        turn_0.events.emplace_back(player_moved);
      }
      for (uint16_t i = 0; i < server_options.initial_blocks; ++i) {
        auto position = generate_position();
        if (blocks.contains(position))
          continue;
        blocks.emplace(position);
        turn_0.events.emplace_back(BlockPlaced{position});
      }
      return turn_0;
    };

    // turn 0
    {
      auto turn_0 = generate_turn_0();
      RLock r_lock(catching_up_mutex);
      Lock lock(clients_mutex);
      send_to_all_clients(turn_0);
      turns.emplace_back(turn_0);
    }

    // turns 1..game_length
    for (uint16_t turn_id = 0; turn_id < server_options.game_length;
         ++turn_id) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(server_options.turn_duration));

      Turn turn;
      turn.turn = uint16_t(turn_id + 1);

      std::set<PlayerId> exploded_players;
      std::set<Position> exploded_blocks;
      std::set<Position> blocks_to_be_placed;

      // updating ticking bombs
      for (auto it = ticking_bombs.begin(); it != ticking_bombs.end();) {
        --it->second.timer;
        if (it->second.timer == 0) {
          BombExploded bomb_exploded;
          bomb_exploded.id = it->first;
          for (auto [dx, dy] : std::vector<std::pair<int, int>>{
                   {1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
            for (int i = 0; i <= server_options.explosion_radius; ++i) {
              int x = it->second.position.x + i * dx;
              int y = it->second.position.y + i * dy;
              if (!is_legal(x, y))
                break;
              Position position = {uint16_t(x), uint16_t(y)};
              for (const auto &[player_id, player_position] :
                   player_positions) {
                if (position == player_position) {
                  bomb_exploded.robots_destroyed.emplace_back(player_id);
                  exploded_players.emplace(player_id);
                }
              }
              if (blocks.contains(position)) {
                bomb_exploded.blocks_destroyed.emplace_back(position);
                exploded_blocks.emplace(position);
                break;
              }
            }
          }
          turn.events.emplace_back(bomb_exploded);
          it = ticking_bombs.erase(it);
        } else {
          ++it;
        }
      }

      // updating scores
      for (const auto &player_id : exploded_players)
        ++scores[player_id];

      // player actions
      {
        Lock lock(client_messages_mutex);
        for (PlayerId player_id = 0; player_id < server_options.players_count;
             ++player_id) {
          if (exploded_players.contains(player_id)) {
            PlayerMoved player_moved;
            player_moved.id = player_id;
            player_moved.position = generate_position();
            turn.events.emplace_back(player_moved);
            player_positions[player_id] = player_moved.position;
          } else {
            auto it = client_messages.find(player_to_socket[player_id]);
            if (it == client_messages.end()) {
              continue;
            }
            auto client_message = it->second;
            if (std::holds_alternative<PlaceBomb>(client_message.m)) {
              auto bomb_id = next_bomb_id++;
              auto position = player_positions[player_id];
              ticking_bombs[bomb_id] = {position, server_options.bomb_timer};
              turn.events.emplace_back(BombPlaced{bomb_id, position});
            } else if (std::holds_alternative<PlaceBlock>(client_message.m)) {
              auto position = player_positions[player_id];
              if (!blocks.contains(position)) {
                blocks_to_be_placed.emplace(position);
                turn.events.emplace_back(BlockPlaced{position});
              }
            } else if (std::holds_alternative<Move>(client_message.m)) {
              auto [x, y] = player_positions[player_id].move(
                  std::get<Move>(client_message.m).direction);
              if (is_legal(x, y)) {
                Position new_position = {uint16_t(x), uint16_t(y)};
                if (!blocks.contains(new_position)) {
                  player_positions[player_id] = new_position;
                  turn.events.emplace_back(
                      PlayerMoved{player_id, new_position});
                }
              }
            }
          }
        }
        client_messages.clear();
      }

      // block changes
      for (const auto &block : exploded_blocks)
        blocks.erase(block);
      for (const auto &block : blocks_to_be_placed)
        blocks.emplace(block);

      // sending Turn
      {
        RLock r_lock(catching_up_mutex);
        Lock lock(clients_mutex);
        send_to_all_clients(turn);
        turns.emplace_back(turn);
      }
    }
    // sending GameEnded
    {
      RLock r_lock(catching_up_mutex);
      Lock lock(clients_mutex);
      send_to_all_clients(GameEnded{scores});
      accepted_players.clear();
      turns.clear();
      is_lobby = true;
    }
  }
}
