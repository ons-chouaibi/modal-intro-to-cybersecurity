The ν-challenge:

The source.c code is the source code provided in the challenge. Since it was stated in the challenge that
the source code of the vm to crack, I tested the source code provided in the challenges with gdb in order 
to find the length of the sequence I needed to inject in **buf** in order to overwrite the content of RIP
(the return address).
The offset calculates is 140.

The file .gdb_history contains the history of the tests I did with gdb. (with the a.txt)

For the **attack.c** code, I created a payload to inject in the **buf** variable; this payload is initialized
by binary codes of **No Operation**; then I injected the binary code of an exec shellcode at the begining of **buf** and
the address of **buf** at buffer+offfset.


Ps: the sequence of the shell code was taken from here **https://www.exploit-db.com/exploits/43550** (I tried to inject
a sequence I generated on my machine via msfvenom but it did not work. I think it was too long (154 bytes))

**To compile the code run make on bash**
**To run the code run ./attack on bash**

**Expected Output**
[+] Received: 0x7ffe318f8900 0x7ffe318f8a00
What is your name?
[+] Buffer sent.

Then the program will not exit and the user can inject their commands.

Ps: the code  was inspired by the course in the video **https://www.youtube.com/watch?v=btkuAEbcQ80**

