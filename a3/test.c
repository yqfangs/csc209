#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

#include "helper.h"
int get_smallest(struct rec *p, int n) {
    int i;
    int smallest_freq = INT_MAX, smallest_index = -1;
    
    for (i = 0; i < n; i++) {
    	if ((p + i)->freq != -1 && (p + i)->freq < smallest_freq) {
		smallest_freq = (p + i)->freq;
		smallest_index = i;
        }
    } 
    return smallest_index;
}

/* wrapper function for close */
void
Close (int fd) {
    if (close(fd) == -1) {
	perror("close");
	exit(1);
    }
}


int main(int argc, char **argv) {
    
    extern char *optarg;
    char ch;
    char *input_file = NULL, *output_file = NULL;
    int i, j, n;
    int rec_size = sizeof(struct rec), file_size;
    int pid;
    
    /* Check for the correct number of arguments. */
    if (argc != 7) {
	fprintf(stderr, "Usage: psort -n <number of processes> "
		"-f <input file name> -o <output file name>\n");
	exit(1);
    }

    /* Read in arguments. */
    while ((ch = getopt(argc, argv, "n:f:o:")) != -1) {
        switch(ch) {
	case 'n':
	    n = atoi(optarg);
	    break;	    
        case 'f':
	    input_file = optarg;
	    break;
	case 'o':
	    output_file = optarg;
	    break;
        default:
            fprintf(stderr, "Usage: psort -n <number of processses> "
		"-f <input file name> -o <output file name>\n");
	    exit(1);
	}
    }

    /* Check if file size is a multiple of the struct rec size. */
    if ((file_size = get_file_size(input_file)) % rec_size != 0) {
    	fprintf(stderr, "%s: invalid input file\n", input_file); 
    	exit(1);
    }

    int num_recs = file_size / rec_size;
    int status, fd[n][2];

    for (i = 0; i < n; i++) {
	printf("loop #%d:\n ", i);
    	if (pipe(fd[i]) == -1) {
	    perror("pipe");
	    exit(1);
	}
	if ((pid = fork()) == -1) {
	    perror("fork");
	    exit(1);
	} else if (pid == 0) { /* child */
	    printf("child number: %d\n", i);
            exit(0);
	}else{
	    if (wait(&status) == -1) { /* child crashed. */
	    perror("wait");
	    exit(1);
	}else{
	    if (WIFEXITED(status)) { /* child failed. */
	    	if (WEXITSTATUS(status) != 0) {
		    fprintf(stderr, "Child process %d terminated prematurely\n", pid);
		}
	    }
	}
	
	}
    }


    return 0;
}