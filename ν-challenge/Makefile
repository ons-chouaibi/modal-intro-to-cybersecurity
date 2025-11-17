all: source attack
source: source.c
	cc -g -fno-stack-protector -z execstack -no-pie source.c -o source
attack:
	cc -g -fno-stack-protector -z execstack attack.c -o attack

clean:
	rm -f source attack