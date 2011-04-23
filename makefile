proxyserver: main.o operaciones.o data.o parser.o log.o
	g++ -lpthread main.o operaciones.o data.o parser.o log.o -o proxyserver 
main.o: main.cpp operaciones.h data.h parser.h log.h
	g++ main.cpp -c 
log.o: log.cpp log.h
	g++ log.cpp -c
parser.o: parser.cpp parser.h
	g++ parser.cpp -c
data.o: data.cpp data.h
	g++ data.cpp -c
operaciones.o: operaciones.cpp operaciones.h
	g++ operaciones.cpp -c 

clean:
	rm -rf *o 
	rm -rf proxyserver


