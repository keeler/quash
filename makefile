DIR_NAME=EECS678-Project1-JeffCailteux-KeelerRussell

quash: quash.o utils.o
	g++ -O3 -g -o quash quash.o utils.o

quash.o: quash.cpp utils.cpp
	g++ -O3 -g -Wall -c quash.cpp

utils.o: utils.cpp
	g++ -O3 -g -Wall -c utils.cpp

tar:
	mkdir $(DIR_NAME)
	cp Command.hpp makefile quash.cpp README report.doc utils.cpp utils.hpp $(DIR_NAME)
	tar -czvf $(DIR_NAME).tar.gz $(DIR_NAME)

clean:
	rm -f quash *.o

