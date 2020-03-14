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
	int b;
	if((b = read(fds[smallest][0], &recs_in[smallest], sizeof(struct rec)) == 0)){//the child is empty
	    recs_in[smallest].freq = -1;
	    num_empty++;
	}else if( b == -1){
	    perror("read");
	    exit(1);
	}
    }	    
	
    if(fclose(fp) != 0){//finish using fp
	perror("fclose");
	exit(1);
    }
    free(recs_in);   
}
void child_process(char *in, int i_process, int num_in_process, int position, int *fd){
    FILE *fp;
    struct rec *recs;
    recs = malloc(sizeof(struct rec) * num_in_process);
    if((fp = fopen(in, "r")) == NULL){
        perror("fopen");
        exit(1);
    }
    
    if((fseek(fp, position * sizeof(struct rec), SEEK_SET) == -1)){
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

    qsort(recs, read, sizeof(struct rec), compare_freq);

    //write to pipe
    for(int i = 0; i < num_in_process; i++){
        if((write(fd[1], &recs[i], sizeof(struct rec)) == -1)){
          perror("write");
          exit(1);
        }
    }
    if ((close(fd[1])) == -1) { //finish writing
	perror("close");
    }
    free(recs); //already wrote to pipe
}

int main(int argc, char **argv) {
    extern char *optarg;
    int num_process;
    int file_size;
    char *infile = NULL, *outfile = NULL;
    int i;
    int num_per_process;
    int current;

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
    if(num_process == 0){
	exit(0);
    }
    
    int **fds;
    fds = malloc(num_process * sizeof(int *));
    for(i = 0; i < num_process; i++){
	fds[i] = malloc(2 *sizeof(int)); 
    }
    //check if the file size can be evenly divided by the size of struct rec to ensure the file contains only struct rec
    if ((file_size = get_file_size(infile)) % sizeof(struct rec) != 0){
        fprintf(stderr, "The input file %s is not in correct format\n", infile);
        exit(1);
    }
    int num_rec = file_size / sizeof(struct rec);
    
    //if num_process > number of struct rec in the file, only fork num_rec times
    if(num_process > num_rec){
	num_process = num_rec;
    }
    
    //find the number of struct rec in each process
    int per_rec_list[num_process];
    num_per_process = num_rec / num_process;
    
    for(i = 0; i < num_process; i++){
	per_rec_list[i] = num_per_process;
	if(i < (num_rec % num_process)){
	    per_rec_list[i] += 1;
	}
    }

    for(i = 0; i < num_process; i++){
        if(pipe(fds[i]) == -1){
            perror("pipe");
            exit(1);
        }
	int r = fork();
	if (r == -1) {
	    perror("fork");
	    exit(1);
	} else if (r == 0) { /* child */
	    //close all file descriptor that will not be used in i-th child process
	    int t;
	    for(int j = 0; j < i; j++){
		if ((t = close(fds[j][0])) == -1) {
		    perror("close");
		}
	    }
	    if ((t = close(fds[i][0])) == -1) { //child does not need to read from pipe
		    perror("close");
	    }
	    
	    current = 0;
	    for(int k = 0; k < i; k++){
		current += per_rec_list[k];
	    }
	    
	    child_process(infile, i, per_rec_list[i], current, fds[i]);
	    
	    for(i = 0; i < num_process; i++){//free malloc in child process
		free(fds[i]);
	    }
	    free(fds);
            exit(0);
	}
    }
    for(i = 0; i < num_process; i++){//close the write end of pipe
	if(close(fds[i][1]) == -1){
	    perror("close");
	}
    }
    merge(outfile, num_process, fds);
    
    for(i = 0; i < num_process; i++){
	free(fds[i]);
    }
    free(fds);
    
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
}
