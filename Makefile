EXECUTABLE=http-server
OBJECTS=tcpserver.o \
	token_list.o \
	message.o \
	response.o \
	request.o \
	event_handler.o \
	html_builder.o \
	message_handler.o \
	test_handler.o \
	file_handler.o \
	request_condition.o \
	to_string_time.o \
	logger.o \
	io_mplex.o \
	epoll_mplex.o

CXX=clang++ -std=c++11
CXXFLAGS=-Wall -O2

http-server : $(OBJECTS)
	$(CXX) -o $(EXECUTABLE) $(OBJECTS)

tcpserver.o : server.hpp tcpserver.cpp
	$(CXX) $(CXXFLAGS) -c tcpserver.cpp

token_list.o : server.hpp token_list.cpp
	$(CXX) $(CXXFLAGS) -c token_list.cpp

message.o : server.hpp message.cpp
	$(CXX) $(CXXFLAGS) -c message.cpp

response.o : server.hpp response.cpp
	$(CXX) $(CXXFLAGS) -c response.cpp

request.o : server.hpp request.cpp
	$(CXX) $(CXXFLAGS) -c request.cpp

event_handler.o : server.hpp event_handler.cpp
	$(CXX) $(CXXFLAGS) -c event_handler.cpp

html_builder.o : server.hpp html_builder.cpp
	$(CXX) $(CXXFLAGS) -c html_builder.cpp

message_handler.o : server.hpp message_handler.cpp
	$(CXX) $(CXXFLAGS) -c message_handler.cpp

test_handler.o : server.hpp test_handler.cpp
	$(CXX) $(CXXFLAGS) -c test_handler.cpp

file_handler.o : server.hpp file_handler.cpp
	$(CXX) $(CXXFLAGS) -c file_handler.cpp

request_condition.o : server.hpp request_condition.cpp
	$(CXX) $(CXXFLAGS) -c request_condition.cpp

to_string_time.o : server.hpp to_string_time.cpp
	$(CXX) $(CXXFLAGS) -c to_string_time.cpp

logger.o : server.hpp logger.cpp
	$(CXX) $(CXXFLAGS) -c logger.cpp

io_mplex.o : server.hpp io_mplex.cpp
	$(CXX) $(CXXFLAGS) -c io_mplex.cpp

epoll_mplex.o : server.hpp epoll_mplex.cpp
	$(CXX) $(CXXFLAGS) -c epoll_mplex.cpp

clean :
	rm -f $(EXECUTABLE) $(OBJECTS)
