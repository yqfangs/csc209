#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include "helper.h"
void merge(char *out, int num_process, int **fds){
    FILE *fp;
    struct rec *recs_in;
    int num_empty = 0;
    int i;
    
    printf("in merge\n");
    recs_in = malloc(sizeof(struct rec) * num_process);
    
    if((fp = fopen(out, "w")) == NULL){//open output file
        perror("fopen");
        exit(1);
    }
    //construct an array that contains the current smallest rec among all child processes
    for(i = 0; i < num_process; i++){
	if((read(fds[i][0], &recs_in[i], sizeof(struct rec)) == -1)){
	    perror("read");
	    exit(1);
	}
	printf("read from pipe: %s %d at index %d\n", recs_in[i].word, recs_in[i].freq, i);
	
    }
    //find the smallest among the smallests and write to output
    
    while(num_empty < num_process){
	int smallest = 0;
	for(i = 0; i < num_process; i++){
	    if(recs_in[i].freq < recs_in[smallest].freq && recs_in[i].freq != -1){
		smallest = i;
	    }
	    if(recs_in[smallest].freq == -1){
		smallest = i;
	    }
	}//found the smallest rec in the current array, write to output file
	if(fwrite(&recs_in[smallest], sizeof(struct rec), 1, fp) != 1){
	    perror("fwrite");
	    exit(1);
	}
	printf("write to file: %s %d\n", recs_in[smallest].word, recs_in[smallest].freq);
	int b;
	if((b = read(fds[smallest][0], &recs_in[smallest], sizeof(struct rec)) == 0)){//the child is empty
	    printf("smallest index: %d\n", smallest);
	    recs_in[smallest].freq = -1;
	    num_empty++;
	}else if( b == -1){
	    perror("read");
	    exit(1);
	}else{
	    printf("read again from pipe: %s %d at index %d\n", recs_in[smallest].word, recs_in[smallest].freq, smallest);
	}
    }	    
	
    if(fclose(fp) != 0){//finish using fp
	perror("fclose");
	exit(1);
    }
    free(recs_in);   
}
void child_process(char *in, int i_process, int num_in_process, int *fd){
    FILE *fp;
    struct rec *recs;
    printf("in child_process\n");
    recs = malloc(sizeof(struct rec) * num_in_process);
    printf("i_process: %d, num_in_process: %d\n", i_process, num_in_process); 
    if((fp = fopen(in, "r")) == NULL){
        perror("fopen");
        exit(1);
    }
    printf("read file opened\n");
    
    if((fseek(fp, i_process * num_in_process * sizeof(struct rec), SEEK_SET) == -1)){
	perror("fseek");
	exit(1);
    }
    
    int read = fread(recs, sizeof(struct rec), num_in_process, fp);
    if(read != num_in_process){
        fprintf(stderr, "fread only read %d objects.\n", read);
        exit(1);
    }
    if(fclose(fp) != 0){//finish using fp
	perror("fclose");
	exit(1);
    }
    printf("--------before qsort----------\n");
    //call qsort
    for(int i = 0; i < num_in_process; i++){
        printf("%s %d\n", recs[i].word, recs[i].freq);
    }
    qsort(recs, read, sizeof(struct rec), compare_freq);
    printf("---------after qsort-------\n");
    for(int i = 0; i < num_in_process; i++){
        printf("%s %d\n", recs[i].word, recs[i].freq);
    }
    //write to pipe
    for(int i = 0; i < num_in_process; i++){
        if((write(fd[1], &recs[i], sizeof(struct rec)) == -1)){
          perror("write");
          exit(1);
        }
	printf("write to pipe: %s %d\n", recs[i].word, recs[i].freq);
    }
    if ((close(fd[1])) == -1) { //finish writing
	perror("close");
    }
    printf("wrote to pipe\n");
    free(recs); //already wrote to pipe
    printf("freed\n");
}

int main(int argc, char **argv) {
    extern char *optarg;
    int num_process;
    int rec_size = sizeof(struct rec);
    int file_size;
    char *infile = NULL, *outfile = NULL;
    int i;
    int num_per_process;

    if (argc != 7) {
        fprintf(stderr, "Usage: psort -n <number of processes> -f <inputfile> -o <outputfile>\n");
        exit(1);
    }
    int ch;
    /* read in arguments */
    while ((ch = getopt(argc, argv, "n:f:o:")) != -1) {
        switch(ch) {
        case 'n':
            num_process = strtol(optarg, NULL, 10);
            break;
        case 'f':
            infile = optarg;
            break;
        case 'o':
            outfile = optarg;
            break;
        default:
            fprintf(stderr, "Usage: psort -n <number of processes> -f <inputfile> -o <outputfile>\n");
            exit(1);
        }
    }
    int **fds;
    fds = malloc(num_process * sizeof(int *));
    for(i = 0; i < num_process; i++){
	fds[i] = malloc(2 *sizeof(int)); 
    }
    //check if the file size can be evenly divided by the size of struct rec to ensure the file contains only struct rec
    if ((file_size = get_file_size(infile)) % rec_size != 0){
        fprintf(stderr, "The input file %s is not in correct format\n", infile);
        exit(1);
    }
    int num_rec = file_size / rec_size;
    
    //if num_process > number of struct rec in the file, only fork num_rec times
    if(num_process > num_rec){
	num_process = num_rec;
    }
    
    for(i = 0; i < num_process; i++){
	printf("loop number: %d\n", i);
        if(pipe(fds[i]) == -1){
            perror("pipe");
            exit(1);
        }
	int r = fork();
	if (r == -1) {
	    perror("fork");
	    exit(1);
	} else if (r == 0) { /* child */
	    printf("----------------------------child number: %d\n", i);
	    //find the number of struct rec in each process
	    num_per_process = num_rec / num_process;
	    if(i < (num_rec % num_process)){
		num_per_process += 1;
	    }
	    printf("num_per_process: %d\n", num_per_process);
	    //close all file descriptor that will not be used in i-th child process
	    int t;
	    for(int j = 0; j < i; j++){
		printf("pipe #: %d\n", j);
		if ((t = close(fds[j][0])) == -1) {
		    perror("close");
		}
		printf("pipe return: %d\n", t);
		printf("close file descriptor %d\n", j); 
	    }
	    if ((t = close(fds[i][0])) == -1) { //child does not need to read from pipe
		    perror("close");
	    }
	    printf("close: %d\n", t);
	    //if(i < (num_rec % num_process)){
		
	    //}else{
		child_process(infile, i, num_per_process, fds[i]);
	    //}
            exit(0);
	}else {
	    close(fds[i][1]);
	}
    }
    
    merge(outfile, num_process, fds);
    
    int status;
    for(i = 0; i < num_process; i++){
	if(wait(&status) == -1){
	    perror("wait");
	}else{
	    if(WIFEXITED(status)){
		if((WEXITSTATUS(status)) != 0){ //check if child terminates correctly
		    fprintf(stderr, "Child terminated abnormally\n");
		}
	    }
	}
    }
    for(i = 0; i < num_process; i++){
	free(fds[i]);
    }
    free(fds);
}
