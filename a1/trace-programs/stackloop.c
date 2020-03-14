#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int stack_loop(int iters) {
	int i;
    unsigned int b[iters];
	for(i = 0; i < iters; i++) {
        b[i] = i * 2;
	}
    return b[0];
}

int main(int argc, char ** argv) {
	/* Markers used to bound trace regions of interest */
	volatile char MARKER_START, MARKER_END;
	/* Record marker addresses */
	FILE* marker_fp = fopen("./marker-stackloop","w");
	if(marker_fp == NULL ) {
		perror("Couldn't open marker file:");
		exit(1);
	}
	fprintf(marker_fp, "%p %p", &MARKER_START, &MARKER_END );
	fclose(marker_fp);

	MARKER_START = 33;
	int out = stack_loop(100);
	MARKER_END = 34;

    printf("First element = %d\n", out);

	return 0;
}
