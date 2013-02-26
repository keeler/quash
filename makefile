quash: quash.o utils.o builtins.o
	g++ -O3 -g -o quash quash.o utils.o builtins.o

quash.o: quash.cpp utils.cpp builtins.cpp
	g++ -O3 -g -Wall -c quash.cpp

builtins.o: builtins.cpp utils.cpp
	g++ -O3 -g -Wall -c builtins.cpp

utils.o: utils.cpp
	g++ -O3 -g -Wall -c utils.cpp

clean:
	rm -f quash *.o

