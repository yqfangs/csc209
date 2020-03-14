all : simple stackloop heaploop matmul 

simple : simple.c
	gcc -Wall -std=gnu99 -g -o simple simple.c

stackloop : stackloop.c
	gcc -Wall -std=gnu99 -g -o stackloop stackloop.c

heaploop : heaploop.c
	gcc -Wall -std=gnu99 -g -o heaploop heaploop.c

matmul : matmul.c
	gcc -Wall -std=gnu99 -g -o matmul matmul.c

traces: heaploop matmul simple stackloop
	./runit simple 
	./runit heaploop
	./runit stackloop
	./runit matmul 32

clean : 
	rm simple stackloop heaploop matmul
	rm full-simple.tr full-matmul.tr full-stackloop.tr full-heaploop.tr
	rm marker-simple marker-matmul marker-stackloop marker-heaploop
