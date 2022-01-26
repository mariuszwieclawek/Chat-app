CC=gcc
CFLAGS=-I.
#CFLAGS=

OBJECTS = multicast_main server client

all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
