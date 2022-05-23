#include<stdio.h>
#include<errno.h>
#include<fcntl.h>

int main(){
    char c;
    c = getchar();
    if(c == 'y'){
        printf("hello");
    }
}