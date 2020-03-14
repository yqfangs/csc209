#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Reads a trace file produced by valgrind and an address marker file produced
 * by the program being traced. Outputs only the memory reference lines in
 * between the two markers
 */

int main(int argc, char **argv) {
    
    if(argc != 3) {
         fprintf(stderr, "Usage: %s tracefile markerfile\n", argv[0]);
         exit(1);
    }
    
    FILE *tracefile = NULL;
    FILE *markerfile = NULL;
    
    // Addresses should be stored in unsigned long variables
    unsigned long start_marker, end_marker;
    unsigned long address;
    char type;
    
    tracefile = fopen(argv[1], "r");
    markerfile = fopen(argv[2], "r");
    
    fscanf(markerfile, "%lx %lx", &start_marker, &end_marker);
    while(fscanf(tracefile, " %c %lx, %*d", &type, &address) == 2 && 
     start_marker != address);
    while(fscanf(tracefile, " %c %lx, %*d", &type, &address) == 2 && 
     end_marker != address){
      printf("%c,%#lx\n", type, address);
      }
      
    /* For printing output, use this exact formatting string where the
     * first conversion is for the type of memory reference, and the second
     * is the address
     */
    //printf("%c,%#lx\n", VARIABLES TO PRINT GO HERE);

    return 0;
}
