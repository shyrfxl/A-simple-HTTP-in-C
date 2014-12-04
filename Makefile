# macros
OBJS = main.o cgi.o error.o http.o logging.o net.o request.o response.o utils.o daemon.o
CC = gcc
DEBUG = -g
LFLAGS = -Wall $(DEBUG)
CFLAGS = -c -Wall -Werror $(DEBUG) 
LIBS = -lmagic
DIR=sws-slytherin

# executables
all: sws

sws: $(OBJS) 
	$(CC) $(LFLAGS) $(OBJS) -o sws $(LIBS)

# object files
main.o: main.c cgi.h net.h request.h daemon.h
	$(CC) $(CFLAGS) main.c

cgi.o: cgi.c cgi.h error.h net.h request.h utils.h
	$(CC) $(CFLAGS) cgi.c

error.o: error.c error.h response.h
	$(CC) $(CFLAGS) error.c

http.o: http.c http.h error.h request.h response.h logging.h
	$(CC) $(CFLAGS) http.c

logging.o: logging.c logging.h 
	$(CC) $(CFLAGS) logging.c

net.o: net.c net.h cgi.h error.h http.h logging.h request.h response.h utils.h
	$(CC) $(CFLAGS) net.c

request.o: request.c request.h net.h utils.h
	$(CC) $(CFLAGS) request.c 

response.o: response.c response.h
	$(CC) $(CFLAGS) response.c

utils.o: utils.c utils.h request.h
	$(CC) $(CFLAGS) utils.c

daemon.o: daemon.c daemon.h
	$(CC) $(CFLAGS) daemon.c

# remove files
clean:
	rm *.o sws

# submit package
tar:
	mkdir $(DIR)
	cp main.c $(DIR)
	cp cgi.c $(DIR)
	cp cgi.h $(DIR)
	cp daemon.c $(DIR)
	cp daemon.h $(DIR)
	cp error.c $(DIR)
	cp error.h $(DIR)
	cp http.c $(DIR)
	cp http.h $(DIR)
	cp logging.c $(DIR)
	cp logging.h $(DIR)
	cp net.c $(DIR)
	cp net.h $(DIR)
	cp request.c $(DIR)
	cp request.h $(DIR)
	cp response.c $(DIR)
	cp response.h $(DIR)
	cp utils.c $(DIR)
	cp utils.h $(DIR)
	cp Makefile $(DIR)
	cp README $(DIR)
	git log > $(DIR)/GitLog
	tar cvf $(DIR).tar $(DIR)/
	rm -r $(DIR)
