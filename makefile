quash: main.o
	g++ -O3 -g -o quash main.o

main.o: main.cpp
	g++ -O3 -g -Wall -c main.cpp

clean:
	rm -f quash *.o

