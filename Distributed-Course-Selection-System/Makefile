VPATH =./src
CPP = g++
CXXFLAGS = -std=c++11  -g 
OBJS = main.o buffer.o HTTPrequest.o HTTPresponse.o HTTPconnection.o parser.o timer.o epoll.o webserver.o  KVDB.o logger.o conf.o ossSocket.o  RUSocket.o TCPServer.o TCPClient.o coordinate.o participant.o 
INCLUDE = -I./include   -I./src
LIB = -pthread 

CS_system : $(OBJS)
	$(CPP) $(CXXFLAGS) -o $@ $(OBJS) $(LIB) $(INCLUDE)
	@rm *.o

main.o : ./src/main.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

buffer.o : ./src/buffer.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

proxyserver.o : ./src/proxyserver.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

HTTPrequest.o : ./src/HTTPrequest.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

HTTPresponse.o : ./src/HTTPresponse.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

HTTPconnection.o : ./src/HTTPconnection.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

timer.o : ./src/timer.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

epoll.o : ./src/epoll.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)	

webserver.o : ./src/webserver.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)	
# json.o : json.hpp
# 	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

KVDB.o : ./src/KVDB.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

logger.o : ./src/logger.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

coordinate.o : ./src/coordinate.cpp
	$(CPP) $(CXXFLAGS) -c $^ $(LIB) $(INCLUDE)

parser.o : ./src/parser.cc
	$(CPP) $(CXXFLAGS) -c $^ $(LIB) $(INCLUDE)

participant.o :  ./src/participant.cpp
	$(CPP) $(CXXFLAGS) -c $^ $(LIB)  $(INCLUDE)

conf.o : ./src/conf.cc
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

ossSocket.o : ./src/ossSocket.cc
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

RUSocket.o : ./src/RUSocket.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

TCPServer.o : ./src/TCPServer.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

TCPClient.o : ./src/TCPClient.cpp
	$(CPP) $(CXXFLAGS) -c $^  $(LIB) $(INCLUDE)

PHONY: clean

clean:
	rm -f ./CS_system ./*.o ./src/*.gch