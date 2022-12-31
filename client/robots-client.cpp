#include <iostream>
#include <set>

#include "client_options.hpp"
#include "deserialize.hpp"
#include "messages.hpp"
#include "serialize.hpp"

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

std::atomic<bool> send_join{true};

void udp_receive(const ClientOptions &client_options,
                 std::unique_ptr<udp::socket> socket_udp_receive,
                 tcp::socket &socket_tcp) {
  message_t recv_buf(3);
  udp::endpoint remote_endpoint;

  for (;;) {
    size_t len;
    try {
      len = socket_udp_receive->receive_from(boost::asio::buffer(recv_buf),
                                             remote_endpoint);
    } catch (...) {
      continue;
    }
    try {
      size_t pos = 0;
      auto input_message = deserialize_input_message(recv_buf, pos);
      if (pos != len)
        throw CouldNotDeserialize();

      ClientMessage client_message;
      if (send_join.load()) {
        client_message.m = Join{client_options.player_name};
      } else if (std::holds_alternative<PlaceBomb>(input_message.m)) {
        client_message.m = PlaceBomb{};
      } else if (std::holds_alternative<PlaceBlock>(input_message.m)) {
        client_message.m = PlaceBlock{};
      } else { // Move
        client_message.m = std::get<Move>(input_message.m);
      }

      auto client_message_serialized = serialize(client_message);
      boost::asio::write(socket_tcp,
                         boost::asio::buffer(client_message_serialized));
    } catch (...) {
    }
  }
}

int main(int argc, char **argv) {
  auto client_options = get_client_options(argc, argv);

  boost::asio::io_context io_context;

  tcp::resolver resolver_tcp(io_context);
  tcp::resolver::results_type endpoints_tcp;
  try {
    endpoints_tcp = resolver_tcp.resolve(
        client_options.server_address,
        std::__cxx11::to_string(client_options.server_port));
  } catch (...) {
    std::cerr << "Invalid server address" << std::endl;
    exit(1);
  }

  tcp::socket socket_tcp(io_context);
  try {
    boost::asio::connect(socket_tcp, endpoints_tcp);
    socket_tcp.set_option(tcp::no_delay(true));
  } catch (...) {
    std::cerr << "Could not connect to the server" << std::endl;
    exit(1);
  }

  boost::asio::io_context io_context_udp;

  std::unique_ptr<udp::socket> socket_udp_receive;
  try {
    socket_udp_receive = std::make_unique<udp::socket>(
        udp::socket(io_context, udp::endpoint(udp::v6(), client_options.port)));
  } catch (...) {
    std::cerr << "Could not bind to the given port" << std::endl;
    exit(1);
  }

  std::thread thread_udp_receive{udp_receive, client_options,
                                 std::move(socket_udp_receive),
                                 std::ref(socket_tcp)};

  udp::resolver resolver_udp_send(io_context);
  udp::endpoint endpoint_udp_send;
  try {
    endpoint_udp_send =
        *resolver_udp_send
             .resolve(client_options.gui_address,
                      std::__cxx11::to_string(client_options.gui_port))
             .begin();
  } catch (...) {
    std::cerr << "Invalid gui address" << std::endl;
    exit(1);
  }

  udp::socket socket_udp_send(io_context);
  try {
    socket_udp_send.open(udp::v6());
  } catch (...) {
    std::cerr << "Could not connect to the gui" << std::endl;
    exit(1);
  }

  TCPReader tcp_reader(socket_tcp);

  Hello hello{};
  std::map<PlayerId, Player> players;
  uint16_t game_turn;
  std::map<PlayerId, Position> player_positions;
  std::set<Position> blocks;
  std::vector<Bomb> bombs;
  std::set<Position> explosions;
  std::map<PlayerId, Score> scores;
  std::map<BombId, Bomb> ticking_bombs;

  for (;;) {
    try {
      message_t gui_message;

      auto make_lobby = [&] {
        Lobby lobby;
        lobby.server_name = hello.server_name;
        lobby.players_count = hello.players_count;
        lobby.size_x = hello.size_x;
        lobby.size_y = hello.size_y;
        lobby.game_length = hello.game_length;
        lobby.explosion_radius = hello.explosion_radius;
        lobby.bomb_timer = hello.bomb_timer;
        lobby.players = players;
        gui_message = serialize(DrawMessage{lobby});
      };
      auto make_game = [&] {
        Game game;
        game.server_name = hello.server_name;
        game.size_x = hello.size_x;
        game.size_y = hello.size_y;
        game.game_length = hello.game_length;
        game.turn = game_turn;
        game.players = players;
        game.player_positions = player_positions;
        game.blocks = std::vector<Position>(blocks.begin(), blocks.end());
        game.bombs = bombs;
        game.explosions =
            std::vector<Position>(explosions.begin(), explosions.end());
        game.scores = scores;
        gui_message = serialize(DrawMessage{game});
      };

      auto server_message = deserialize_server_message(tcp_reader);

      if (std::holds_alternative<Hello>(server_message.m)) {
        hello = get<Hello>(server_message.m);
        make_lobby();
      } else if (std::holds_alternative<AcceptedPlayer>(server_message.m)) {
        auto accepted_player = get<AcceptedPlayer>(server_message.m);
        players[accepted_player.id] = accepted_player.player;
        scores[accepted_player.id] = 0;
        make_lobby();
      } else if (std::holds_alternative<GameStarted>(server_message.m)) {
        auto game_started = get<GameStarted>(server_message.m);

        players = game_started.players;
        for (const auto &[player_id, _player] : players)
          scores[player_id] = 0;

        send_join.store(false);
      } else if (std::holds_alternative<Turn>(server_message.m)) {
        auto turn = get<Turn>(server_message.m);
        game_turn = turn.turn;
        std::set<PlayerId> exploded_players;
        explosions.clear();
        for (auto &[_bomb_id, bomb] : ticking_bombs)
          --bomb.timer;
        for (const auto &event : turn.events) {
          if (std::holds_alternative<BombPlaced>(event.m)) {
            auto bomb_placed = get<BombPlaced>(event.m);
            ticking_bombs[bomb_placed.id] =
                Bomb{bomb_placed.position, hello.bomb_timer};
          } else if (std::holds_alternative<BombExploded>(event.m)) {
            auto bomb_exploded = get<BombExploded>(event.m);

            auto position = ticking_bombs[bomb_exploded.id].position;
            auto is_legal = [&](int x, int y) {
              return x >= 0 && x < hello.size_x && y >= 0 && y < hello.size_y;
            };
            std::set<Position> exploded_blocks(
                bomb_exploded.blocks_destroyed.begin(),
                bomb_exploded.blocks_destroyed.end());
            for (auto [dx, dy] : std::vector<std::pair<int, int>>{
                     {1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
              for (int i = 0; i <= hello.explosion_radius; ++i) {
                int x = position.x + i * dx;
                int y = position.y + i * dy;
                if (!is_legal(x, y))
                  break;
                explosions.emplace(x, y);
                if (exploded_blocks.contains({uint16_t(x), uint16_t(y)}))
                  break;
              }
            }

            ticking_bombs.erase(bomb_exploded.id);
            for (const auto &robot : bomb_exploded.robots_destroyed)
              exploded_players.emplace(robot);
            for (const auto &block : bomb_exploded.blocks_destroyed)
              blocks.erase(block);
          } else if (std::holds_alternative<PlayerMoved>(event.m)) {
            auto player_moved = get<PlayerMoved>(event.m);
            player_positions[player_moved.id] = player_moved.position;
          } else { // BlockPlaced
            auto block_placed = get<BlockPlaced>(event.m);
            blocks.emplace(block_placed.position);
          }
        }
        for (const auto &player_id : exploded_players)
          ++scores[player_id];
        bombs.clear();
        for (const auto &[_bomb_id, bomb] : ticking_bombs)
          bombs.emplace_back(bomb);

        make_game();
      } else { // GameEnded
        players.clear();
        player_positions.clear();
        blocks.clear();
        bombs.clear();
        explosions.clear();
        scores.clear();
        ticking_bombs.clear();

        make_lobby();
        send_join.store(true);
      }

      if (!gui_message.empty())
        socket_udp_send.send_to(boost::asio::buffer(gui_message),
                                endpoint_udp_send);
    } catch (...) {
      std::cerr << "Connection to the server closed" << std::endl;
      exit(1);
    }
  }
}
