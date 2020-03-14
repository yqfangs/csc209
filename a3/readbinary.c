#include <stdio.h>
#include <stdlib.h>
#include "helper.h"

int main(int argc, char *argv[]) {
    FILE * fp;
    struct rec in;
    if(argc !=2){
        fprintf(stderr, "Usage: readbinary filename\n");
        exit(1);
    }
    if((fp = fopen(argv[1], "r")) == NULL){
        perror("fopen");
        exit(1);
    }
    while(fread(&in, sizeof(struct rec), 1, fp) == 1){
        printf("%s %d\n", in.word, in.freq);
    }
    
    return 0;
    
               
}