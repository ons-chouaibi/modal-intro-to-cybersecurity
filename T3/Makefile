LINK_TARGET = server client

OBJS = \
	server.o\
	client.o\
	

REBUILDABLES = $(OBJS) $(LINK_TARGET)

all : $(LINK_TARGET)
clean: 
	rm -f $(REBUILDABLES)

% : %.o
	cc -g3 -lpthread -o  $@ $^ 

%.o : %.c
	cc -g3  -Wall -o $@ -c $<
  