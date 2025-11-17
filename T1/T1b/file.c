#include<stdio.h>

int main()
{
   char *s = "Hello";
   
   while(*s!=NULL)
   printf("%c", *s++);
}