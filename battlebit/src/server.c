//
// Created by carson on 5/20/20.
//

#include "stdio.h"
#include "stdlib.h"
#include "server.h"
#include "char_buff.h"
#include "game.h"
#include "repl.h"
#include "pthread.h"
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h>    //inet_addr
#include<unistd.h>    //write

static game_server *SERVER;
static pthread_mutex_t  *LOCK;

void init_server() {
    if (SERVER == NULL) {
        SERVER = calloc(1, sizeof(struct game_server));
        LOCK = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(LOCK, NULL);

        //This needs to go somewhere to stop collisions of changing game state.
        //pthread_mutex_lock(LOCK);
        //code
        //pthread_mutex_unlock(LOCK);
    } else {
        printf("Server already started");
    }
}

int handle_client_connect(int player) {
    // STEP 9 - This is the big one: you will need to re-implement the REPL code from
    // the repl.c file, but with a twist: you need to make sure that a player only
    // fires when the game is initialized and it is there turn.  They can broadcast
    // a message whenever, but they can't just shoot unless it is their turn.
    //
    // The commands will obviously not take a player argument, as with the system level
    // REPL, but they will be similar: load, fire, etc.
    //
    // You must broadcast informative messages after each shot (HIT! or MISS!) and let
    // the player print out their current board state at any time.
    //
    // This function will end up looking a lot like repl_execute_command, except you will
    // be working against network sockets rather than standard out, and you will need
    // to coordinate turns via the game::status field.

    player = player-1;
    char raw_buffer[2000];
    char_buff *input_buffer = cb_create(2000);
    char_buff *output_buffer = cb_create(2000);

    int player_socket_fd = SERVER->player_sockets[player];
    pthread_t p1 = SERVER->player_threads[0];

    int read_size;
    cb_append(output_buffer,"Welcome to the battleBit server Player ");
    if(player == 0){
        cb_append(output_buffer,"0");
    }else{
        cb_append(output_buffer,"1");
    }
    cb_append(output_buffer, "\nbattleBit (? for help) > ");
    cb_write(player_socket_fd, output_buffer);

    while ((read_size = recv(player_socket_fd, raw_buffer, 2000, 0)) > 0){
        //reset our buffers
        cb_reset(output_buffer);
        cb_reset(input_buffer);
        if(read_size > 0) {
            raw_buffer[read_size] = '\0'; //null terminate

            //append to input buffer
            cb_append(input_buffer, raw_buffer);

            //tokenize
            char *command = cb_tokenize(input_buffer, " \r\n");

            if (strcmp(command, "?") == 0) {
                //create output
                cb_append(output_buffer,
                          "? - show help\nload <string> - load a ship layout file for the given player\nshow  - shows the board for the given player\nfire [0-7] [0-7] - fire at the given position\nsay <string> - Send the string to all players as part of a chat\nexit - quit the server\n");
                //output it
                cb_write(player_socket_fd, output_buffer);
            } else if (strcmp(command, "load") == 0) {
                char* board = cb_next_token(input_buffer);
                game_load_board(game_get_current(),player,board);
                if(game_get_current()->players[1].ships == 0){
                    cb_append(output_buffer,"Waiting on Player 1");
                    cb_write(player_socket_fd,output_buffer);
                }else{
                    cb_append(output_buffer,"All Player Boards Loaded\nPlayer 0 Turn");
                    cb_write(player_socket_fd,output_buffer);
                }
                //cb_print(board);
            } else if (strcmp(command, "show") == 0) {
                struct char_buff *buffer = cb_create(2000);
                repl_print_board(game_get_current(),player,buffer);
                cb_append(output_buffer,buffer);
                cb_write(player_socket_fd,buffer);
            } else if (strcmp(command, "fire") == 0) {
                game* theGame = game_get_current();

                enum game_status stat;
                if(player == 0){
                    stat = PLAYER_0_TURN;
                }else{
                    stat = PLAYER_1_TURN;
                }

                if(theGame->players[1].ships == 0 && theGame->players[0].ships ==0){
                    cb_append(output_buffer, "Game Has Not Begun!");
                    cb_write(player_socket_fd,output_buffer);
                }else if(theGame->status != stat){
                    cb_append(output_buffer, "Player ");
                    if(player == 0){
                        cb_append(output_buffer, "1 Turn");
                    }else{
                        cb_append(output_buffer, "0 Turn");
                    }
                    cb_write(player_socket_fd,output_buffer);
                }else{
                    char* x_s = cb_next_token(input_buffer);
                    char* y_s = cb_next_token(input_buffer);

                    int x_val = x_s[0] - '0';
                    int y_val = y_s[0] - '0';

                    pthread_mutex_lock(LOCK);
                    int success = game_fire(game_get_current(),player,x_val,y_val);
                    pthread_mutex_unlock(LOCK);

                    cb_append(output_buffer, "\nPlayer ");
                    if(player == 0){
                        cb_append(output_buffer, "0 fires at ");
                    }else{
                        cb_append(output_buffer, "1 fires at ");
                    }

                    cb_append(output_buffer, x_s);
                    cb_append(output_buffer, " ");
                    cb_append(output_buffer, y_s);
                    cb_append(output_buffer, " - ");

                    if(success == 0){
                        cb_append(output_buffer,"MISS!\n");
                    }else if (game_get_current()->status == PLAYER_1_WINS){
                        cb_append(output_buffer, "HIT! - PLAYER 1 WINS!\n");
                    }else if (game_get_current()->status == PLAYER_0_WINS){
                        cb_append(output_buffer, "HIT! - PLAYER 0 WINS!\n");
                    }else{
                        cb_append(output_buffer,"HIT!\n");
                    }

                    server_broadcast(output_buffer);

                    if(player == 0){
                        cb_reset(output_buffer);
                        cb_append(output_buffer, "\nbattleBit (? for help) > ");
                        cb_write(SERVER->player_sockets[1], output_buffer);
                    }else{
                        cb_reset(output_buffer);
                        cb_append(output_buffer, "\nbattleBit (? for help) > ");
                        cb_write(SERVER->player_sockets[0], output_buffer);
                    }

                }

            } else if (strcmp(command, "say") == 0) {
                char* word = cb_next_token(input_buffer);

                cb_append(output_buffer, "\nPlayer ");
                if(player ==0){
                    cb_append(output_buffer, "0 says: ");
                }else{
                    cb_append(output_buffer, "1 says: ");
                }

                int x = 0;
                while(x != 1){
                    if(word == NULL){
                        if(player == 0){
                            cb_write(SERVER->player_sockets[1],output_buffer);
                            cb_append(output_buffer, "\n");
                            setbuf(stdout,NULL);
                            printf("%s",output_buffer->buffer);
                            //cb_print(output_buffer);
                            cb_reset(output_buffer);
                            cb_append(output_buffer, "\nbattleBit (? for help) > ");
                            cb_write(SERVER->player_sockets[1], output_buffer);

                        }else{
                            cb_write(SERVER->player_sockets[0],output_buffer);
                            cb_append(output_buffer, "\n");
                            setbuf(stdout,NULL);
                            printf("%s",output_buffer->buffer);
                            //cb_print(output_buffer);
                            cb_reset(output_buffer);
                            cb_append(output_buffer, "\nbattleBit (? for help) > ");
                            cb_write(SERVER->player_sockets[0], output_buffer);
                        }
                        x = 1;
                    }else{
                        cb_append(output_buffer, word);
                        cb_append(output_buffer, " ");
                        word = cb_next_token(input_buffer);
                    }
                }
            } else if (strcmp(command, "exit") == 0) {
                close(player_socket_fd);
            } else if (command != NULL) {
                //output creation
                cb_append(output_buffer, "Command was : ");
                cb_append(output_buffer, command);
                //output it
                cb_write(player_socket_fd, output_buffer);
            }
            cb_reset(output_buffer);
            cb_append(output_buffer, "\nbattleBit (? for help) > ");
            cb_write(player_socket_fd, output_buffer);

        }
    }

}

void server_broadcast(char_buff *msg) {
    // send message to all players
    for(int i=0; i<2;i++){
        int player_fd = SERVER->player_sockets[i];
        cb_write(player_fd,msg);
    }
    //how do I print to server
    setbuf(stdout,NULL);
    printf("%s",msg->buffer);
}

int run_server() {
    // STEP 8 - implement the server code to put this on the network.
    // Here you will need to initalize a server socket and wait for incoming connections.
    //
    // When a connection occurs, store the corresponding new client socket in the SERVER.player_sockets array
    // as the corresponding player position.
    //
    // You will then create a thread running handle_client_connect, passing the player number out
    // so they can interact with the server asynchronously

    int server_socket_fd = socket(AF_INET,
                                  SOCK_STREAM,
                                  IPPROTO_TCP);
    if (server_socket_fd == -1) {
        printf("Could not create socket\n");
    }

    int yes = 1;
    setsockopt(server_socket_fd,
               SOL_SOCKET,
               SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in server;

    // fill out the socket information
    server.sin_family = AF_INET;
    // bind the socket on all available interfaces
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9876);

    int request = 0;
    if (bind(server_socket_fd,
            // Again with the cast
             (struct sockaddr *) &server,
             sizeof(server)) < 0) {
        puts("Bind failed");
    } else {
        puts("Bind worked!");
        listen(server_socket_fd, 88);

        //Accept an incoming connection
        puts("Waiting for incoming connections...");


        struct sockaddr_in client;
        socklen_t size_from_connect;
        int client_socket_fd;

        int playNum = 0;
        while ((client_socket_fd = accept(server_socket_fd,
                                          (struct sockaddr *) &client,
                                          &size_from_connect)) > 0) {

            SERVER->player_sockets[playNum] = client_socket_fd;

            if(playNum==0){
                playNum++;
                pthread_create(&SERVER->player_threads[0], NULL, handle_client_connect,playNum);
            }else{
                playNum++;
                pthread_create(&SERVER->player_threads[1],NULL,handle_client_connect,playNum);
            }

            //Fixed, dont call handle_client_connect(playNum) in the pthread_create function
            //printf("I MADE IT HERE");

            //4 total threads
            //    1: main thread, make server start create a pthread to call run_server()
            //    2/3: within run server (that has the while loop) make a pthread to do handle_client_connect
            //    4?: IDK
        }
    }
}

int server_start() {
    // STEP 7 - using a pthread, run the run_server() function asynchronously, so you can still
    // interact with the game via the command line REPL

    //how to make server run on dif thread?
    //pthread_t mainS;
    //pthread_create(&mainS,NULL,init_server,NULL);
    //fork();

    init_server();
    pthread_create(&SERVER->server_thread, NULL, (void *) run_server, NULL);


}
