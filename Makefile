#
# Вообщето здессь даже можно писать коментарии
#

CC = gcc
CFLAGS = -g -Wall -ansi -pedantic
OBJS = editor.o\
	   text_database.o\
	   interface_controller.o\
	   terminal_controller.o\
	   file_handler.o\
	   miscellaneous_stuff.o

BINARIES = ted

all: $(BINARIES)

ted: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o : %.c %h
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f $(OBJS) $(BINARIES) *.h.gch

