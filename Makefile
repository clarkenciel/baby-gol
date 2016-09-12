gol: main.cpp
	clang++ -Wall -std=c++11 -pthread -o gol main.cpp

clean:
	$(RM) gol
