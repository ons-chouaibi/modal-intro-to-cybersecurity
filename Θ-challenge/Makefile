all: dhcp starving web dns

dhcp: dhcp.c
	gcc -Wall -o dhcp dhcp.c
starving: starving.c
	gcc -Wall -o starving starving.c
dns: dns.c
	gcc -Wall -o dns dns.c
web: web.c
	gcc -Wall -o web web.c -lpthread
clean:
	rm -f starving dhcp dns web
