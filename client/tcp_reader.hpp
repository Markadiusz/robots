#ifndef __TCP_READER_HPP
#define __TCP_READER_HPP

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <queue>

#include "messages.hpp"

struct InvalidTCPMessage : public std::exception {
  const char *what() const throw() { return "InvalidTCPMessage"; }
};

class TCPReader {
public:
  TCPReader(boost::asio::ip::tcp::socket &socket_) : socket(socket_) {}

  message_t read(size_t cnt) {
    message_t ret;
    transfer(ret, cnt);
    while (ret.size() < cnt) {
      boost::system::error_code error;
      size_t len = socket.read_some(boost::asio::buffer(buffer), error);

      for (size_t i = 0; i < len; ++i)
        queue.emplace(buffer[i]);

      transfer(ret, cnt);

      if (error == boost::asio::error::eof)
        break;
      else if (error)
        throw InvalidTCPMessage();
    }
    if (ret.size() < cnt)
      throw InvalidTCPMessage();
    return ret;
  }

private:
  static constexpr size_t BUFFER_SIZE = 66'000;
  boost::asio::ip::tcp::socket &socket;
  boost::array<uint8_t, BUFFER_SIZE> buffer;
  std::queue<uint8_t> queue;

  void transfer(message_t &ret, size_t cnt) {
    while (!queue.empty() && ret.size() < cnt) {
      ret.emplace_back(queue.front());
      queue.pop();
    }
  }
};

#endif // __TCP_READER_HPP
