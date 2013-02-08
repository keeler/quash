quash: quash.o
	g++ -O3 -g -o quash quash.o

quash.o: quash.cpp
	g++ -O3 -g -Wall -c quash.cpp

clean:
	rm -f quash *.o

