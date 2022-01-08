CC=gcc
CFLAGS=-I.
#CFLAGS=

OBJECTS = multicast_main udp_daytime_cli_br udp_daytime_cli_listen6 udp_daytime_cli_listen udp_daytime_srvv4_br udp_daytime_srvv4

all: $(OBJECTS)

$(OBJECTS):%:%.c
	@echo Compiling $<  to  $@
	$(CC) -o $@ $< $(CFLAGS)

	
clean:
	rm  $(OBJECTS) 
