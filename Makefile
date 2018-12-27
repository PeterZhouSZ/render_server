SRC=./src/main.cpp
INCLUDE=./external/
all:
	g++ -std=c++11 -o renderer $(SRC) -I$(INCLUDE)