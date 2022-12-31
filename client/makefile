CFLAGS = -Wall -Wextra -Wconversion -Werror -O2 -std=gnu++20
BOOSTFLAGS = -lboost_system -lpthread -lboost_thread -lboost_program_options
CC = g++

robots-client: robots-client.o serialize.o deserialize.o client_options.o
	$(CC) -o $@ robots-client.o serialize.o deserialize.o client_options.o $(BOOSTFLAGS)

robots-client.o: robots-client.cpp messages.hpp serialize.hpp deserialize.hpp client_options.hpp
	$(CC) $(CFLAGS) -c robots-client.cpp $(BOOSTFLAGS)

serialize.o: serialize.cpp serialize.hpp messages.hpp
	$(CC) $(CFLAGS) -c serialize.cpp

deserialize.o: deserialize.cpp deserialize.hpp messages.hpp tcp_reader.hpp
	$(CC) $(CFLAGS) -c deserialize.cpp $(BOOSTFLAGS)

client_options.o: client_options.cpp client_options.hpp
	$(CC) $(CFLAGS) -c client_options.cpp $(BOOSTFLAGS)

clean:
	-rm -f *.o robots-client
