all: hack rst_attack

hack: hack.c
	cc -g -Wall hack.c -o hack -lpcap

rst_attack: rst_attack.c
	cc -g -Wall rst_attack.c -o rst_attack -lpcap
clean:
	rm -f hack rst_attack