Targets= server client fancyclient
all: $(Targets)
server: server.c
	cc -Wall -o $@ $<
client: client.c
	cc -Wall -o $@ $<
fancyclient: fancyclient.c
	cc -Wall -o $@ $<
clean:
	rm -f $(Targets)