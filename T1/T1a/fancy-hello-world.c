#include<stdio.h>
#include<string.h>
#include "fancy-hello-world.h"

int main(void){
    printf("Enter your name: \n");
    char name[51];
    char output[70];
    fgets(name,sizeof(name),stdin);
    hello_string(name,output);
    printf("%s", output);
    return 0;
}

void hello_string(char* name, char* output){
    strcpy(output, "Hello World, hello ");
    strcat(output,name);

}
