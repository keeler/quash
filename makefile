quash: quash.o utils.o
	g++ -O3 -g -o quash quash.o utils.o

quash.o: quash.cpp utils.cpp
	g++ -O3 -g -Wall -c quash.cpp

utils.o: utils.cpp
	g++ -O3 -g -Wall -c utils.cpp

clean:
	rm -f quash *.o

