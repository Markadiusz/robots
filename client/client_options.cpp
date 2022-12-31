#include <boost/numeric/conversion/cast.hpp>
#include <boost/program_options.hpp>
#include <iostream>

#include "client_options.hpp"

ClientOptions get_client_options(int argc, char **argv) {
  namespace po = boost::program_options;
  try {
    po::options_description desc("Allowed options");
    desc.add_options()(
        "gui-address,d", po::value<std::string>(),
        "<(host name):(port) or (IPv4):(port) or (IPv6):(port)>")("help,h", "")(
        "player-name,n", po::value<std::string>(),
        "<String>")("port,p", po::value<uint16_t>(), "<u16>")(
        "server-address,s", po::value<std::string>(),
        "<(host name):(port) or (IPv4):(port) or (IPv6):(port)>");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      exit(0);
    }

    ClientOptions ret;
    std::vector<std::string> missing_options;

    auto check_option = [&]<typename T>(const std::string &s, T &elem) {
      if (vm.count(s)) {
        elem = vm[s].as<T>();
      } else {
        missing_options.emplace_back(s);
      }
    };

    check_option("player-name", ret.player_name);
    check_option("port", ret.port);

    auto throw_invalid_address = [](const std::string &s) {
      throw std::runtime_error(s + " is not a valid address");
    };
    auto split_address =
        [&](const std::string &s) -> std::pair<std::string, uint16_t> {
      auto last_colon = s.rfind(":");
      if (last_colon == std::string::npos) {
        throw_invalid_address(s);
      }
      uint16_t port;
      try {
        port = boost::numeric_cast<uint16_t>(boost::lexical_cast<int>(
            s.substr(last_colon + 1, s.length() - last_colon - 1)));
      } catch (...) {
        throw_invalid_address(s);
      }
      if (last_colon != 0 && s.at(0) == '[' && s.at(last_colon - 1) == ']')
        return {s.substr(1, last_colon - 2), port};
      else
        return {s.substr(0, last_colon), port};
    };
    auto check_address = [&](const std::string &s, std::string &address,
                             uint16_t &port) {
      if (vm.count(s)) {
        std::tie(address, port) = split_address(vm[s].as<std::string>());
      } else {
        missing_options.emplace_back(s);
      }
    };

    check_address("gui-address", ret.gui_address, ret.gui_port);
    check_address("server-address", ret.server_address, ret.server_port);

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
