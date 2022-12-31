#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>

#include "server_options.hpp"

ServerOptions get_server_options(int argc, char *argv[]) {
  namespace po = boost::program_options;
  try {
    po::options_description desc("Allowed options");
    desc.add_options()("bomb-timer,b", po::value<uint16_t>(), "<u16>")(
        "players-count,c", po::value<uint16_t>(), "<u8>")(
        "turn-duration,d", po::value<uint64_t>(), "<u64, milliseconds>")(
        "explosion-radius,e", po::value<uint16_t>(), "<u16>")("help,h", "")(
        "initial-blocks,k", po::value<uint16_t>(),
        "<u16>")("game-length,l", po::value<uint16_t>(),
                 "<u16>")("server-name,n", po::value<std::string>(),
                          "<String>")("port,p", po::value<uint16_t>(), "<u16>")(
        "seed,s", po::value<uint32_t>(), "<u32, optional parameter>")(
        "size-x,x", po::value<uint16_t>(),
        "<u16>")("size-y,y", po::value<uint16_t>(), "<u16>");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      exit(0);
    }

    ServerOptions ret;
    ret.seed =
        uint32_t(std::chrono::system_clock::now().time_since_epoch().count());
    std::vector<std::string> missing_options;

    auto check_option =
        [&]<typename T>(const std::string &s, T &elem, bool required = true) {
      if (vm.count(s)) {
        elem = vm[s].as<T>();
      } else if (required) {
        missing_options.emplace_back(s);
      }
    };

    check_option("bomb-timer", ret.bomb_timer);
    check_option("players-count", ret.players_count);
    check_option("turn-duration", ret.turn_duration);
    check_option("explosion-radius", ret.explosion_radius);
    check_option("initial-blocks", ret.initial_blocks);
    check_option("game-length", ret.game_length);
    check_option("server-name", ret.server_name);
    check_option("port", ret.port);
    check_option("seed", ret.seed, false);
    check_option("size-x", ret.size_x);
    check_option("size-y", ret.size_y);

    constexpr int players_count_limit = (1 << 8);
    if (vm.count("players-count") && ret.players_count >= players_count_limit) {
      throw std::runtime_error("the argument ('" +
                               std::__cxx11::to_string(ret.players_count) +
                               "') for option '--players-count' is invalid");
    }

    if (missing_options.empty()) {
      return ret;
    } else {
      std::cerr << "Missing options:\n";
      for (auto option : missing_options) {
        std::cerr << "  --" << option << '\n';
      }
      std::cerr << std::endl;
      exit(1);
    }
  } catch (std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    exit(1);
  } catch (...) {
    std::cerr << "Exception of unknown type!" << std::endl;
    exit(1);
  }
}
