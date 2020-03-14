#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Constants that determine that address ranges of different memory regions

#define GLOBALS_START 0x400000
#define GLOBALS_END   0x700000
#define HEAP_START   0x4000000
#define HEAP_END     0x8000000
#define STACK_START 0xfff000000

int main(int argc, char **argv) {
    
    FILE *fp = NULL;

    if(argc == 1) {
        fp = stdin;

    } else if(argc == 2) {
        fp = fopen(argv[1], "r");
        if(fp == NULL){
            perror("fopen");
            exit(1);
        }
    } else {
        fprintf(stderr, "Usage: %s [tracefile]\n", argv[0]);
        exit(1);
    }

    /* Complete the implementation */
    char type;
    unsigned long address;
    int ins, mod, load, store, global, heap, stack;
    ins = 0;
    mod = 0;
    load = 0;
    store = 0;
    global = 0;
    heap = 0;
    stack = 0;
    
    while (fscanf(fp, " %c,%lx", &type, &address) == 2){
        if (type == 'I'){
            ins++;
        }else if(type == 'M'){
            mod++;
        }else if(type == 'L'){
            load++;
        }else if(type == 'S'){
            store++;
        }
        if (type != 'I'){
            if(address >= GLOBALS_START && address <= GLOBALS_END){
                global++;
            }else if (address >= HEAP_START && address <= HEAP_END){
                heap++;
            }else if (address >= STACK_START){
                stack++;
            }
        }
    }
            

    /* Use these print statements to print the ouput. It is important that 
     * the output match precisely for testing purposes.
     * Fill in the relevant variables in each print statement.
     * The print statements are commented out so that the program compiles.  
     * Uncomment them as you get each piece working.
     */
     
    printf("Reference Counts by Type:\n");
    printf("    Instructions: %d\n", ins);
    printf("    Modifications: %d\n", mod);
    printf("    Loads: %d\n", load);
    printf("    Stores: %d\n", store);
    printf("Data Reference Counts by Location:\n");
    printf("    Globals: %d\n", global);
    printf("    Heap: %d\n", heap);
    printf("    Stack: %d\n", stack);

    return 0;
}
