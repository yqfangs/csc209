#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_state(char *state, int size);
void update_state(char *state, int size);


int main(int argc, char **argv) {

    if (argc != 3) {
    	fprintf(stderr, "Usage: USAGE: life initial n\n");
    	return 1;
    }

    int size = strlen(argv[1]);
    
    // TODO: complete the main function
    int num = strtol(argv[2], NULL, 10);
    int i;
    char state[size];
    
    for(i = 0; i < size; i++){
	state[i] = argv[1][i];
    }	    
	    
    for(i = 0; i < num; i++){
	    print_state(state, size);
	    update_state(state, size);
    }
    return 0;
}