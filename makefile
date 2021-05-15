CFLAGS=-std=c11 -pedantic -Wall -Wvla -Werror -D_GNU_SOURCE -D_POSIX_C_SOURCE

all: client server maint stat

client: utils_v10.h  utils_v10.o client/client.c
	cc $(CFLAGS) -o client/client client/client.c utils_v10.o  

server: server/serverUtil.h utils_v10.h  utils_v10.o server/server.c
	cc $(CFLAGS) -o server/server server/server.c utils_v10.o

maint: server/serverUtil.h utils_v10.h  utils_v10.o server/maint.c
	cc $(CFLAGS) -o server/maint server/maint.c utils_v10.o

stat: server/serverUtil.h utils_v10.h  utils_v10.o server/stat.c
	cc $(CFLAGS) -o server/stat server/stat.c utils_v10.o

utils_v10.o: messages.h utils_v10.h utils_v10.c 
	cc $(CFLAGS) -c utils_v10.c 

# for dev purpose
clear:
	clear
	
clean:
	rm -f *.o
	rm -f client/client
	rm -f server/server
	rm -f server/maint
	rm -f server/stat
	rm -f output/*
	ipcrm -a

cleanAll:
	rm -f *.o
	rm -f client/client
	rm -f server/server
	rm -f server/maint
	rm -f server/stat
	rm -f output/*
	rm -f programs/*
	ipcrm -a
