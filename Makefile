SRC :=src/data.cpp src/main.cpp
STD :=c++17
all:
	g++ -std=$(STD) $(SRC) -o build/ssbs
