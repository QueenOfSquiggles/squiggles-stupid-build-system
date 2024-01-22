SRC :=src/data.cpp src/display.cpp src/main.cpp 
STD :=c++17
all:
	g++ -std=$(STD) $(SRC) -o build/ssbs
	./run_test.sh