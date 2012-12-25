
all: load gui migrate

load: load.o
	gcc load.o -o load

load.o:	load.c
	gcc -I../include -c load.c

gui: gui.o info.pb-c.o
	gcc -g gui.o info.pb-c.o -lprotobuf-c -lcdk -lm -lncurses -o gui

gui.o: gui.c
	gcc -c -I../include gui.c

migrate: migrate.o info.pb-c.o
	gcc migrate.o info.pb-c.o -lprotobuf-c -pthread -o migrate

migrate.o: migrate.c
	gcc -c -I../include migrate.c

info.pb-c.o: info.pb-c.c
	gcc -c -I../include info.pb-c.c
	
clean:
	rm -fr *o load gui migrate
