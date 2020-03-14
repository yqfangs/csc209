#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "family.h"

/* Number of word pointers allocated for a new family.
   This is also the number of word pointers added to a family
   using realloc, when the family is full.
*/
static int family_increment = 0;


/* Set family_increment to size, and initialize random number generator.
   The random number generator is used to select a random word from a family.
   This function should be called exactly once, on startup.
*/
void init_family(int size) {
    family_increment = size;
    srand(time(0));
}


/* Given a pointer to the head of a linked list of Family nodes,
   print each family's signature and words.

   Do not modify this function. It will be used for marking.
*/
void print_families(Family* fam_list) {
    int i;
    Family *fam = fam_list;
    
    while (fam) {
        printf("***Family signature: %s Num words: %d\n",
               fam->signature, fam->num_words);
        for(i = 0; i < fam->num_words; i++) {
            printf("     %s\n", fam->word_ptrs[i]);
        }
        printf("\n");
        fam = fam->next;
    }
}


/* Return a pointer to a new family whose signature is 
   a copy of str. Initialize word_ptrs to point to 
   family_increment+1 pointers, numwords to 0, 
   maxwords to family_increment, and next to NULL.
*/
Family *new_family(char *str) {
    Family *fam = malloc(sizeof(Family));
    if (fam == NULL) {
        perror("malloc");
        exit(1);
    }
    if(str != NULL){
    fam->signature = malloc(sizeof(char) * (strlen(str) + 1));
        if (fam->signature == NULL) {
        perror("malloc");
        exit(1);
    }
    strcpy(fam->signature, str);
    fam->signature[strlen(str)] = '\0';
    }
    
    fam->word_ptrs = malloc(sizeof(char*) * (family_increment + 1));
    if (fam->word_ptrs == NULL) {
        perror("malloc");
        exit(1);
    }
    fam->word_ptrs[0] = NULL;
    fam->num_words = 0;
    fam->max_words = family_increment;
    fam->next = NULL;
    return fam;
}


/* Add word to the next free slot fam->word_ptrs.
   If fam->word_ptrs is full, first use realloc to allocate family_increment
   more pointers and then add the new pointer.
*/
void add_word_to_family(Family *fam, char *word) {
    if((fam->num_words + 1) == fam->max_words){
        fam->word_ptrs = realloc(fam->word_ptrs, (family_increment + fam->max_words) * sizeof(char*));
        if (fam->word_ptrs == NULL) {
            perror("realloc");
            exit(1);
        }
        fam->max_words += family_increment;
    }
    fam->word_ptrs[fam->num_words] = word;
    fam->word_ptrs[fam->num_words + 1] = NULL;
    fam->num_words += 1;
}


/* Return a pointer to the family whose signature is sig;
   if there is no such family, return NULL.
   fam_list is a pointer to the head of a list of Family nodes.
*/
Family *find_family(Family *fam_list, char *sig) {
    Family *curr = fam_list;
    while(curr != NULL && strcmp(curr->signature,sig) != 0){
        curr = curr->next;
    }
    return curr;
}


/* Return a pointer to the family in the list with the most words;
   if the list is empty, return NULL. If multiple families have the most words,
   return a pointer to any of them.
   fam_list is a pointer to the head of a list of Family nodes.
*/
Family *find_biggest_family(Family *fam_list) {
    Family *curr = fam_list;
    int max = 0;
    Family *max_node = NULL;
    while(curr != NULL){
        if(curr->num_words > max){
            max_node = curr;
            max = curr->num_words;
        }
        curr = curr->next;
        
    }
    return max_node;
}


/* Deallocate all memory rooted in the List pointed to by fam_list. */
void deallocate_families(Family *fam_list) {
    Family *curr = fam_list;
    Family *previous = NULL;
    while(curr != NULL){
        free(curr->word_ptrs);
        free(curr->signature);
        previous = curr;
        curr = curr->next;
        free(previous);
    }
}


/* Generate and return a linked list of all families using words pointed to
   by word_list, using letter to partition the words.

   Implementation tips: To decide the family in which each word belongs, you
   will need to generate the signature of each word. Create only the families
   that have at least one word from the current word_list.
*/
Family *generate_families(char **word_list, char letter) {
    int i = 0;
    int word_len = strlen(word_list[i]);
    char *signature = malloc(sizeof(char) * (word_len + 1));
    Family *famhead = NULL;
    Family *found = NULL;
    Family *curr = famhead;

    while(word_list[i] != NULL){
        for(int j = 0; j < word_len; j++){ 
            if(word_list[i][j] == letter){
                signature[j] = letter;
            }else{
                signature[j] = '-';
            }
        }
        signature[word_len] = '\0';
        found = find_family(famhead, signature);
        if(found == NULL){
            famhead = new_family(signature);
            famhead->next = curr;
            curr = famhead;
            add_word_to_family(curr, word_list[i]);
        }else{
            add_word_to_family(found,word_list[i]);
        }
        i++;
    }
    free(signature);    
    return famhead;
}


/* Return the signature of the family pointed to by fam. */
char *get_family_signature(Family *fam) {
    return fam->signature;
}


/* Return a pointer to word pointers, each of which
   points to a word in fam. These pointers should not be the same
   as those used by fam->word_ptrs (i.e. they should be independently malloc'd),
   because fam->word_ptrs can move during a realloc.
   As with fam->word_ptrs, the final pointer should be NULL.
*/
char **get_new_word_list(Family *fam) {
    char **new_word_ptrs = malloc(sizeof(char*) * (fam->num_words + 1));
    int i;
    for(i = 0; i < fam->num_words; i++){
        new_word_ptrs[i] = fam->word_ptrs[i];    
    }
    new_word_ptrs[fam->num_words] = NULL;
    return new_word_ptrs;
}


/* Return a pointer to a random word from fam. 
   Use rand (man 3 rand) to generate random integers.
*/
char *get_random_word_from_family(Family *fam) {
    srand(time(0));
    int num = (rand() % fam->num_words);
    char *word = (fam->word_ptrs)[num];
    return word;
}
