#include <stdio.h>
#include <stdlib.h>

int i;
int j;

int main() {
	/* Markers used to bound trace regions of interest */
	volatile char MARKER_START, MARKER_END;
	/* Record marker addresses */
	FILE* marker_fp = fopen("./marker-simple","w");
	if(marker_fp == NULL ) {
		perror("Couldn't open marker file:");
		exit(1);
	}
	fprintf(marker_fp, "%p %p", &MARKER_START, &MARKER_END );
	fclose(marker_fp);

	MARKER_START = 33;
	i = 10;
	j = 20;
	int k = i + j;
	MARKER_END = 34;

    printf("%d\n", k);
	return 0;
}
