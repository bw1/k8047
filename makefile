all: k8047 libk8047.o

k8047: main.c libk8047.o
	gcc main.c libk8047.o -o k8047

libk8047.o: libk8047.c
	gcc -c libk8047.c
