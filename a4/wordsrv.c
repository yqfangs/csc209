#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include "socket.h"
#include "gameplay.h"


#ifndef PORT
    #define PORT 52128
#endif
#define MAX_QUEUE 5


void add_player(struct client **top, int fd, struct in_addr addr);
void remove_player(struct client **top, int fd);

/* These are some of the function prototypes that we used in our solution 
 * You are not required to write functions that match these prototypes, but
 * you may find the helpful when thinking about operations in your program.
 */
/* Send the message in outbuf to all clients */
void broadcast(struct game_state *game, char *outbuf);
void announce_turn(struct game_state *game);
void announce_winner(struct game_state *game, struct client *winner);
/* Move the has_next_turn pointer to the next active client */
void advance_turn(struct game_state *game);


/* The set of socket descriptors for select to monitor.
 * This is a global variable because we need to remove socket descriptors
 * from allset when a write to a socket fails.
 */
fd_set allset;


/* Add a client to the head of the linked list
 */
void add_player(struct client **top, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));

    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));

    p->fd = fd;
    p->ipaddr = addr;
    p->name[0] = '\0';
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *top;
    *top = p;
}

/* Removes client from the linked list and closes its socket.
 * Also removes socket descriptor from allset 
 */
void remove_player(struct client **top, int fd) {
    struct client **p;

    for (p = top; *p && (*p)->fd != fd; p = &(*p)->next)
        ;
    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        FD_CLR((*p)->fd, &allset);
        close((*p)->fd);
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n",
                 fd);
    }
}
void remove_new_player(struct client **top, struct client *player) {
    struct client **p;

    for (p = top; *p && (*p) != player; p = &(*p)->next)
        ;
    if (*p) {
        struct client *t = (*p)->next;
        *p = t;
    }
}


int find_network_newline(const char *buf, int n){
    for(int i = 0; i < n - 1; i++){
        if(buf[i] == '\r' && buf[i+1] == '\n'){
            return i + 2;
        }
    }
    return -1;
}

int read_input(struct client *player, char *input){
    int in = 0;
    int nbytes;
    while((nbytes = read(player->fd, player->in_ptr, MAX_BUF - in)) > 0) {
        in += nbytes;
        int where = find_network_newline(player->inbuf, in);
        if(where > 0) {
            player->inbuf[where - 2] = '\0';
            strcpy(input, player->inbuf);
            return 0;
        }
        player->in_ptr = player->inbuf + in;
    }
    return -1;
}

void handle_username(struct client **new_players, struct client *player, struct game_state *game, int fd, char *in){
    for(struct client *active = game->head; active != NULL; active = active->next){ //check if the name is already taken
        if(strcmp(active->name, in) == 0){
            char *exist_player = EXIST_PLAYER;
            if(write(player->fd, exist_player, strlen(exist_player)) == -1) {
                fprintf(stderr, "Write to client %s failed\n", inet_ntoa(player->ipaddr));
                remove_player(new_players, player->fd);
            }
            break;
        }
    }
    strncpy(player->name, in, MAX_NAME);
    //remove from new_players and add to active player link list
    player->next = game->head;
    game->head = player;
    remove_new_player(new_players, player);
    
    //announce new player joined
    char *new_announce = malloc(sizeof(char) * MAX_MSG);
    if(strlen(player->name) > 0){
        strcpy(new_announce, player->name);
        char *join_announce = JOIN_ANNOUNCE;
        strcat(new_announce, join_announce);
        strcat(new_announce, "\r\n");
        broadcast(game, new_announce);
    }
    
    printf("%s%s\n", player->name, JOIN_ANNOUNCE);
    
    //tell the new active player the current game state
    char *status = malloc(sizeof(char) *MAX_MSG);
    status = status_message(status, game);
    write(player->fd, status, strlen(status));
    
}

void broadcast(struct game_state *game, char *outbuf){
    for(struct client *p = game->head; p != NULL; p = p->next) {
        int num_write = write(p->fd, outbuf, strlen(outbuf));
        if (num_write != strlen(outbuf)) {
            perror("write to client");
            exit(1);
        } 
    }
}

int valid_letter(struct game_state *game, struct client *player, char *in, int fd){
    int ascii = (int)*in;
    game->guesses_left--;
    char *ptr = strstr(game->word, in);
    if(!ptr){
        char not_in[MAX_MSG];
        strcpy(not_in, in);
        strcat(not_in, NOT_IN);
        if(write(fd, not_in, strlen(not_in)) == -1) {
            fprintf(stderr, "Write to client %s failed\n", inet_ntoa(player->ipaddr));
            remove_player(&(game->head), fd);
        }
        advance_turn(game);
    }else{
        for(int j = 0; j < strlen(game->word); j++){ 
            if(game->word[j] == ascii){
                game->guess[j] = ascii;
            }else{
                game->guess[j] = '-';
            }
        }
    }
    char is_in[MAX_MSG];
    strcpy(is_in, player->name);
    strcat(is_in, " guesses: ");
    strcat(is_in, in);
    strcat(is_in, "\r\n");
    broadcast(game, is_in);
    
    char *status = malloc(sizeof(char) *MAX_MSG);
    status = status_message(status, game);
    broadcast(game, status);
    
    return 0;
}

int handle_game_input(struct game_state *game, struct client *player, char *in, int fd){
    char *single_char = SINGLE_CHAR;
    int index;
    int ascii;
    if(strlen(in) != 1 && (strcmp(in, "a") > 0) && (strcmp(in, "z") < 0)){
        if(write(fd, single_char, strlen(single_char)) == -1) {
            fprintf(stderr, "Write to client %s failed\n", inet_ntoa(player->ipaddr));
            remove_player(&(game->head), fd);
        }
        return -1;
    }else{
        ascii = (int)*in;
        index = ascii - 'a';
        if(game->letters_guessed[index] == 1){
            char *letter_guessed = LETTER_GUESSED;
            if(write(fd, letter_guessed, strlen(letter_guessed)) == -1) {
                fprintf(stderr, "Write to client %s failed\n", inet_ntoa(player->ipaddr));
                remove_player(&(game->head), fd);
            }
            return -1;
        }else{
            game->letters_guessed[index] = 1;
            valid_letter(game, player, in, fd);
            return 0;
        }
    }
}

int handle_not_turn_input(struct game_state *game, struct client *player, char *in, int fd){
    if(strlen(in) > 1){
        char *not_turn = NOT_TURN;
        if(write(fd, not_turn, strlen(not_turn)) == -1) {
            fprintf(stderr, "Write to client %s failed\n", inet_ntoa(player->ipaddr));
            remove_player(&game->head, player->fd);
        }
    }
    return 0;
}
void advance_turn(struct game_state *game){
    struct client *p = game->head;
    if(p->next == NULL){
        game->has_next_turn = game->head;
    }else{
        for(p = game->head; p->next != NULL; p = p->next){
            if(p->next == game->has_next_turn){
                game->has_next_turn = p;
                break;
            }
        }
    }
    /*write((game->head)->fd, (game->has_next_turn)->name, strlen((game->has_next_turn)->name));*/
}

int game_over(struct game_state *game){
    if(game->guesses_left == 0 && strchr(game->guess, '-') == NULL){
        return 0;
    }else{
        return 1;
    }
}

int main(int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;
    
    if(argc != 2){
        fprintf(stderr,"Usage: %s <dictionary filename>\n", argv[0]);
        exit(1);
    }
    
    // Create and initialize the game state
    struct game_state game;

    srandom((unsigned int)time(NULL));
    // Set up the file pointer outside of init_game because we want to 
    // just rewind the file when we need to pick a new word
    game.dict.fp = NULL;
    game.dict.size = get_file_length(argv[1]);

    init_game(&game, argv[1]);
    
    // head and has_next_turn also don't change when a subsequent game is
    // started so we initialize them here.
    game.head = NULL;
    game.has_next_turn = NULL;
    
    /* A list of client who have not yet entered their name.  This list is
     * kept separate from the list of active players in the game, because
     * until the new playrs have entered a name, they should not have a turn
     * or receive broadcast messages.  In other words, they can't play until
     * they have a name.
     */
    struct client *new_players = NULL;
    
    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, MAX_QUEUE);
    
    // initialize allset and add listenfd to the
    // set of file descriptors passed into select
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    // maxfd identifies how far into the set to search
    maxfd = listenfd;

    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            continue;
        }

        if (FD_ISSET(listenfd, &rset)){
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_player(&new_players, clientfd, q.sin_addr);
            char *greeting = WELCOME_MSG;
            if(write(clientfd, greeting, strlen(greeting)) == -1) {
                fprintf(stderr, "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                remove_player(&(game.head), p->fd);
            };
        }
        

        /* Check which other socket descriptors have something ready to read.
         * The reason we iterate over the rset descriptors at the top level and
         * search through the two lists of clients each time is that it is
         * possible that a client will be removed in the middle of one of the
         * operations. This is also why we call break after handling the input.
         * If a client has been removed the loop variables may not longer be 
         * valid.
         */
        int cur_fd;
        for(cur_fd = 0; cur_fd <= maxfd; cur_fd++) {
                
            if(FD_ISSET(cur_fd, &rset)) {
                // Check if this socket descriptor is an active player
                
                for(p = game.head; p != NULL; p = p->next) {
                    if(cur_fd == p->fd) {
                        //TODO - handle input from an active client
                        char *in = malloc(MAX_BUF * sizeof(char));
                        memset(in, '\0', MAX_BUF);
                        read_input(p, in);
                        if(game.has_next_turn == p){
                            handle_game_input(&game, p, in, cur_fd);
                        }else{
                            handle_not_turn_input(&game, p, in, cur_fd);
                        }
                        
                        break;
                    }
                }
        
                // Check if any new players are entering their names
                for(p = new_players; p != NULL; p = p->next) {
                    if(cur_fd == p->fd) {
                        // TODO - handle input from an new client who has
                        // not entered an acceptable name.
                        char *in = malloc(MAX_BUF * sizeof(char));
                        memset(in, '\0', MAX_BUF);
                        read_input(p, in);
                        handle_username(&new_players, p, &game, cur_fd, in);
                        if(p->next == NULL){
                            advance_turn(&game);
                        }
                        break;
                    } 
                }
            }

        }
    }
    return 0;
}


