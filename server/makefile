CFLAGS = -Wall -Wextra -Wconversion -Werror -O2 -std=gnu++20
BOOSTFLAGS = -lboost_system -lpthread -lboost_thread -lboost_program_options
CC = g++

robots-server: robots-server.o serialize.o deserialize.o server_options.o
	$(CC) -o $@ robots-server.o serialize.o deserialize.o server_options.o $(BOOSTFLAGS)

robots-server.o: robots-server.cpp messages.hpp serialize.hpp deserialize.hpp server_options.hpp
	$(CC) $(CFLAGS) -c robots-server.cpp $(BOOSTFLAGS)

serialize.o: serialize.cpp serialize.hpp messages.hpp
	$(CC) $(CFLAGS) -c serialize.cpp

deserialize.o: deserialize.cpp deserialize.hpp messages.hpp tcp_reader.hpp
	$(CC) $(CFLAGS) -c deserialize.cpp $(BOOSTFLAGS)

server_options.o: server_options.cpp server_options.hpp
	$(CC) $(CFLAGS) -c server_options.cpp $(BOOSTFLAGS)

clean:
	-rm -f *.o robots-server
