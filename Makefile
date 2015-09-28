PROGRAM=http-server
OBJECTS=http-response.o \
	http-request.o \
	http-connection.o \
	http-condition.o \
	decode-simple-token.o \
	decode-token.o \
	decode-content-length.o \
	decode-etag.o \
	decode-request-line.o \
	decode-request-header.o \
	decode-chunk.o \
	time_to_string.o \
	time_decode.o \
	logger.o \
	html-builder.o \
	handler.o \
	handler-file.o \
	handler-test.o \
	mplex-io.o \
	mplex-epoll.o \
	tcpserver.o

CXX=clang++ -std=c++11
CXXFLAGS=-Wall -O2

$(PROGRAM) : $(OBJECTS)
	$(CXX) -o $(PROGRAM) $(OBJECTS)

http-response.o : http.hpp http-response.cpp
	$(CXX) $(CXXFLAGS) -c http-response.cpp

http-request.o : http.hpp http-request.cpp
	$(CXX) $(CXXFLAGS) -c http-request.cpp

http-connection.o : server.hpp http-connection.cpp
	$(CXX) $(CXXFLAGS) -c http-connection.cpp

http-condition.o : http.hpp http-condition.cpp
	$(CXX) $(CXXFLAGS) -c http-condition.cpp

decode-simple-token.o : http.hpp decode-lookup-cls.hpp decode-simple-token.cpp
	$(CXX) $(CXXFLAGS) -c decode-simple-token.cpp

decode-token.o : http.hpp decode-lookup-cls.hpp decode-token.cpp
	$(CXX) $(CXXFLAGS) -c decode-token.cpp

decode-content-length.o : http.hpp decode-lookup-cls.hpp decode-content-length.cpp
	$(CXX) $(CXXFLAGS) -c decode-content-length.cpp

decode-etag.o : http.hpp decode-lookup-cls.hpp decode-etag.cpp
	$(CXX) $(CXXFLAGS) -c decode-etag.cpp

decode-request-line.o : http.hpp decode-lookup-cls.hpp decode-request-line.cpp
	$(CXX) $(CXXFLAGS) -c decode-request-line.cpp

decode-request-header.o : http.hpp decode-lookup-cls.hpp decode-request-header.cpp
	$(CXX) $(CXXFLAGS) -c decode-request-header.cpp

decode-chunk.o : http.hpp decode-lookup-cls.hpp decode-chunk.cpp
	$(CXX) $(CXXFLAGS) -c decode-chunk.cpp

time_to_string.o : http.hpp time_to_string.cpp
	$(CXX) $(CXXFLAGS) -c time_to_string.cpp

time_decode.o : http.hpp time_decode.cpp
	$(CXX) $(CXXFLAGS) -c time_decode.cpp

logger.o : http.hpp logger.cpp
	$(CXX) $(CXXFLAGS) -c logger.cpp

html-builder.o : html-builder.hpp html-builder.cpp
	$(CXX) $(CXXFLAGS) -c html-builder.cpp

handler.o : server.hpp handler.cpp
	$(CXX) $(CXXFLAGS) -c handler.cpp

handler-file.o : server.hpp handler-file.cpp
	$(CXX) $(CXXFLAGS) -c handler-file.cpp

handler-test.o : server.hpp handler-test.cpp
	$(CXX) $(CXXFLAGS) -c handler-test.cpp

mplex-io.o : server.hpp mplex-io.cpp
	$(CXX) $(CXXFLAGS) -c mplex-io.cpp

mplex-epoll.o : server.hpp mplex-epoll.cpp
	$(CXX) $(CXXFLAGS) -c mplex-epoll.cpp

tcpserver.o : server.hpp tcpserver.cpp
	$(CXX) $(CXXFLAGS) -c tcpserver.cpp

# TESTS

TEST02=tests/02.decode-simple-token.t
TEST02SPEC=tests/02.decode-simple-token.cpp
TEST02OBJ=decode-simple-token.o

TEST03=tests/03.decode-token.t
TEST03SPEC=tests/03.decode-token.cpp
TEST03OBJ=decode-token.o

TEST04=tests/04.decode-content-length.t
TEST04SPEC=tests/04.decode-content-length.cpp
TEST04OBJ=decode-content-length.o

TEST05=tests/05.decode-request.t
TEST05SPEC=tests/05.decode-request.cpp
TEST05OBJ=http-request.o decode-request-line.o decode-request-header.o

TEST06=tests/06.decode-chunk.t
TEST06SPEC=tests/06.decode-chunk.cpp
TEST06OBJ=decode-chunk.o

TEST07=tests/07.decode-etag.t
TEST07SPEC=tests/07.decode-etag.cpp
TEST07OBJ=decode-etag.o

TEST08=tests/08.http-condition.t
TEST08SPEC=tests/08.http-condition.cpp
TEST08OBJ=http-condition.o decode-etag.o time_decode.o

TESTS=$(TEST02) \
	$(TEST03) \
	$(TEST04) \
	$(TEST05) \
	$(TEST06) \
	$(TEST07) \
	$(TEST08)

test : $(TESTS)
	for i in $(TESTS); do echo $$i; $$i; done

.PHONY : build-tests

build-tests : $(TESTS)

$(TEST02) : $(TEST02SPEC) $(TEST02OBJ)
	$(CXX) $(CXXFLAGS) -o $(TEST02) $(TEST02SPEC) $(TEST02OBJ)

$(TEST03) : $(TEST03SPEC) $(TEST03OBJ)
	$(CXX) $(CXXFLAGS) -o $(TEST03) $(TEST03SPEC) $(TEST03OBJ)

$(TEST04) : $(TEST04SPEC) $(TEST04OBJ)
	$(CXX) $(CXXFLAGS) -o $(TEST04) $(TEST04SPEC) $(TEST04OBJ)

$(TEST05) : $(TEST05SPEC) $(TEST05OBJ)
	$(CXX) $(CXXFLAGS) -o $(TEST05) $(TEST05SPEC) $(TEST05OBJ)

$(TEST06) : $(TEST06SPEC) $(TEST06OBJ)
	$(CXX) $(CXXFLAGS) -o $(TEST06) $(TEST06SPEC) $(TEST06OBJ)

$(TEST07) : $(TEST07SPEC) $(TEST07OBJ)
	$(CXX) $(CXXFLAGS) -o $(TEST07) $(TEST07SPEC) $(TEST07OBJ)

$(TEST08) : $(TEST08SPEC) $(TEST08OBJ)
	$(CXX) $(CXXFLAGS) -o $(TEST08) $(TEST08SPEC) $(TEST08OBJ)

.PHONY : clean

clean :
	rm -f $(PROGRAM) $(OBJECTS) $(TESTS)
