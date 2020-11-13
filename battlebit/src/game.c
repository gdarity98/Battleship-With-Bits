//
// Created by carson on 5/20/20.
//

#include <stdlib.h>
#include <stdio.h>
#include "game.h"

// STEP 10 - Synchronization: the GAME structure will be accessed by both players interacting
// asynchronously with the server.  Therefore the data must be protected to avoid race conditions.
// Add the appropriate synchronization needed to ensure a clean battle.

static game * GAME = NULL;

long long int power(int base, int exp){
    int i;
    long long int number = 1;

    for(i=0;i<exp;++i){
        number *= base;
    }

    long long int newNum = number;
    return  newNum;
}

void game_init() {
    if (GAME) {
        free(GAME);
    }
    GAME = malloc(sizeof(game));
    GAME->status = CREATED;
    game_init_player_info(&GAME->players[0]);
    game_init_player_info(&GAME->players[1]);
}

void game_init_player_info(player_info *player_info) {
    player_info->ships = 0;
    player_info->hits = 0;
    player_info->shots = 0;
}

int game_fire(game *game, int player, int x, int y) {
    // Step 5 - This is the crux of the game.  You are going to take a shot from the given player and
    // update all the bit values that store our game state.
    //
    //  - You will need up update the players 'shots' value
    //  - you You will need to see if the shot hits a ship in the opponents ships value.  If so, record a hit in the
    //    current players hits field
    //  - If the shot was a hit, you need to flip the ships value to 0 at that position for the opponents ships field
    //
    //  If the opponents ships value is 0, they have no remaining ships, and you should set the game state to
    //  PLAYER_1_WINS or PLAYER_2_WINS depending on who won.

    //Checking for Invalid Shots
    if(x < 0 || y < 0){
        return(0);
    }
    if(x > 7 || y > 7){
        return(0);
    }

    //Setting up holders for my sanity
    long long int shotHolder = game->players[player].shots;
    long long int hitHolder = game->players[player].hits;
    unsigned long long int theShot = xy_to_bitval(x,y);

    //Updates shots value
    //IDK if this if statement is needed.
    if(theShot & shotHolder){
        return(0);
    }else{
        shotHolder ^= theShot;
        game->players[player].shots = shotHolder;
    }

    //Sets opponent (the person being fired at)
    int opponent = NULL;
    if(player == 0){
        opponent = 1;
    }else{
        opponent = 0;
    }

    //Gets the opponents ships and check if there is a ship to hit
    //If there is then update p[layer hits and opponent ships
    //otherwise change to 0
    long long int opponentShips = game->players[opponent].ships;
    if(theShot & opponentShips){
        //update hits
        hitHolder ^= theShot;
        game->players[player].hits = hitHolder;

        //update opponentShips
        opponentShips ^= theShot;
        game->players[opponent].ships = opponentShips;

        //change to next player turn
        if(player == 0){
            game->status = PLAYER_1_TURN;
        }else{
            game->status = PLAYER_0_TURN;
        }

        //check if player won
        if(game->players[opponent].ships == 0){
            if(player == 0){
                game->status = PLAYER_0_WINS;
            }else{
                game->status = PLAYER_1_WINS;
            }
        }

        return(1);
    }else{
        return(0);
    }
}

unsigned long long int xy_to_bitval(int x, int y) {
    //COMPLETE (made a power function)
    // Step 1 - implement this function.  We are taking an x, y position
    // and using bitwise operators, converting that to an unsigned long long
    // with a 1 in the position corresponding to that x, y
    //
    // x:0, y:0 == 0b00000...0001 (the one is in the first position)
    // x:1, y: 0 == 0b00000...10 (the one is in the second position)
    // ....
    // x:0, y: 1 == 0b100000000 (the one is in the eighth position)
    //
    // you will need to use bitwise operators and some math to produce the right
    // value.
    long long int new_x;
    switch(y){
        case 0: //row 1
            new_x = power(2,x);
            break;
        case 1: //row 2
            new_x = power(2,x+8);
            break;
        case 2: //row 3
            new_x = power(2,x+16);
            break;
        case 3: //row 4
            new_x = power(2,x+24);
            break;
        case 4: //row 5
            new_x = power(2,x+32);
            break;
        case 5: //row 6
            new_x = power(2,x+40);
            break;
        case 6: //row 7
            new_x = power(2,x+48);
            break;
        case 7: //row 8
            if(x==7){
                unsigned long long int IDK = 1ull;
                IDK = IDK << 63ull;
                return IDK;
            }else{
                int exp = x+56;
                new_x = power(2,exp);
                break;
            }
            break;
        default: //catches incorrect input
            new_x = 0;
    }

    if(x == 8 || x <=-1){ //catches incorrect input
        new_x = 0;
    }

    char buffer[67] = {0};
    buffer[0] = '0';
    buffer[1] = '0';
    buffer[66] = '\0';
    unsigned long long int answer;

    // fill out remaining 32 bits, 1 or 0 depending on the value in the number i
    unsigned long long int mask = 1ull<<63;

    for(int j = 2; j <= 65; j++){
        if(mask & new_x){
            buffer[j] = '1';
            mask = mask >> 1u;
        }else{
            buffer[j] = '0';
            mask = mask >> 1u;
        }
        //printf(buffer);
    }


    char *ptr;
    answer = strtoll(buffer, &ptr,2);
    return answer;
}

struct game * game_get_current() {
    return GAME;
}

int game_load_board(struct game *game, int player, char * spec) {
    //COMPLETE
    // Step 2 - implement this function.  Here you are taking a C
    // string that represents a layout of ships, then testing
    // to see if it is a valid layout (no off-the-board positions
    // and no overlapping ships)
    //

    // if it is valid, you should write the corresponding unsigned
    // long long value into the Game->players[player].ships data
    // slot and return 1
    //
    // if it is invalid, you should return -1

    //Set game status to player 0 turn if not player 0 game load board
    if(player != 0){
        game->status = PLAYER_0_TURN;
    }
    //checks if null
    if(spec == NULL){
        return -1;
    }

    //checks for duplicate ships and unwanted chars
    int c = 0;
    int b = 0;
    int d = 0;
    int s = 0;
    int p = 0;

    int i = 0;
    while(spec[i] != '\0') {
        switch (spec[i]) {
            case 'c':
                c++;
                i++;
                if (c > 1) {
                    return -1;
                }
                break;
            case 'C':
                c++;
                i++;
                if (c > 1) {
                    return -1;
                }
                break;
            case 'b':
                b++;
                i++;
                if (b > 1) {
                    return -1;
                }
                break;
            case 'B':
                b++;
                i++;
                if (b > 1) {
                    return -1;
                }
                break;
            case 'd':
                d++;
                i++;
                if (d > 1) {
                    return -1;
                }
                break;
            case 'D':
                d++;
                i++;
                if (d > 1) {
                    return -1;
                }
                break;
            case 's':
                s++;
                i++;
                if (s > 1) {
                    return -1;
                }
                break;
            case 'S':
                s++;
                i++;
                if (s > 1) {
                    return -1;
                }
                break;
            case 'p':
                p++;
                i++;
                if (p > 1) {
                    return -1;
                }
                break;
            case 'P':
                p++;
                i++;
                if (p > 1) {
                    return -1;
                }
                break;
            case '0':
                i++;
                break;
            case '1':
                i++;
                break;
            case '2':
                i++;
                break;
            case '3':
                i++;
                break;
            case '4':
                i++;
                break;
            case '5':
                i++;
                break;
            case '6':
                i++;
                break;
            case '7':
                i++;
                break;
            default:
                return -1;
        }
    }

    //checks length of instruction.
    if((c+b+d+s+p) != 5){
        return -1;
    }


    //adding correctly formatted string ships, checks if they overlap and if they are placed on the board correctly
    i = 0;
    int addedCorrectly = 0;
    //int ia = a - '0';
    char cX = ' ';
    char cY = ' ';
    int iX;
    int iY;
    int check;
    while(spec[i] != '\0'){
        char theChar = spec[i];
        switch(spec[i]){
            case 'c':
                cX = spec[i+1];
                iX = cX-'0';
                cY = spec[i+2];
                iY = cY-'0';
                check = add_ship_vertical(&game->players[player],iX,iY,5);
                if(check == -1){
                    return -1;
                }
                i = i + 3;
                break;
            case 'C':
                cX = spec[i+1];
                iX = cX-'0';
                cY = spec[i+2];
                iY = cY-'0';
                check = add_ship_horizontal(&game->players[player],iX,iY,5);
                if(check == -1){
                    return -1;
                }
                i = i + 3;
                break;
            case 'b':
                cX = spec[i+1];
                iX = cX-'0';
                cY = spec[i+2];
                iY = cY-'0';
                check = add_ship_vertical(&game->players[player],iX,iY,4);
                if(check == -1){
                    return -1;
                }
                i = i + 3;
                break;
            case 'B':
                cX = spec[i+1];
                iX = cX-'0';
                cY = spec[i+2];
                iY = cY-'0';
                check = add_ship_horizontal(&game->players[player],iX,iY,4);
                if(check == -1){
                    return -1;
                }
                i = i + 3;
                break;
            case 'd':
                cX = spec[i+1];
                iX = cX-'0';
                cY = spec[i+2];
                iY = cY-'0';
                check = add_ship_vertical(&game->players[player],iX,iY,3);
                if(check == -1){
                    return -1;
                }
                i = i + 3;
                break;
            case 'D':
                cX = spec[i+1];
                iX = cX-'0';
                cY = spec[i+2];
                iY = cY-'0';
                check = add_ship_horizontal(&game->players[player],iX,iY,3);
                if(check == -1){
                    return -1;
                }
                i = i + 3;
                break;
            case 's':
                cX = spec[i+1];
                iX = cX-'0';
                cY = spec[i+2];
                iY = cY-'0';
                check = add_ship_vertical(&game->players[player],iX,iY,3);
                if(check == -1){
                    return -1;
                }
                i = i + 3;
                break;
            case 'S':
                cX = spec[i+1];
                iX = cX-'0';
                cY = spec[i+2];
                iY = cY-'0';
                check = add_ship_horizontal(&game->players[player],iX,iY,3);
                if(check == -1){
                    return -1;
                }
                i = i + 3;
                break;
            case 'p':
                cX = spec[i+1];
                iX = cX-'0';
                cY = spec[i+2];
                iY = cY-'0';
                check = add_ship_vertical(&game->players[player],iX,iY,2);
                if(check == -1){
                    return -1;
                }
                i = i + 3;
                break;
            case 'P':
                cX = spec[i+1];
                iX = cX-'0';
                cY = spec[i+2];
                iY = cY-'0';
                check = add_ship_horizontal(&game->players[player],iX,iY,2);
                if(check == -1){
                    return -1;
                }
                i = i + 3;
                break;
            default:
                return -1;
        }
    }

    return 1;
}

int add_ship_horizontal(player_info *player, int x, int y, int length) {
    //COMPLETE
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively

        //Should I check both x and y? ASKKKK
    if((x+length) > 8 && length != 0){
        return -1;
    }

    if(length != 0){
        unsigned long long int placement = xy_to_bitval(x,y);
        unsigned long long int mask = 1ull;
        unsigned long long int shipsHolder = player->ships;

        //going to check for overlaps
        if(placement & shipsHolder){
            return -1;
        }else{
            //add to ships, which means flip the correct bit in ships, and return 1
            shipsHolder ^= placement;
            player->ships = shipsHolder;
            add_ship_horizontal(player, x+1,y,length-1);
            //return 1;
        }
    }else{
        return 1;
    }

}

int add_ship_vertical(player_info *player, int x, int y, int length) {
    //COMPLETE
    // implement this as part of Step 2
    // returns 1 if the ship can be added, -1 if not
    // hint: this can be defined recursively

    if((y+length) > 8 && length != 0){
        return -1;
    }

    if(length != 0){
        unsigned long long int placement = xy_to_bitval(x,y);
        unsigned long long int shipsHolder = player->ships;

        //going to check for overlaps
        if(placement & shipsHolder){
            return-1;
        }else{
            //add to ships, which means flip the correct bit in ships, and return 1
            shipsHolder ^= placement;
            player->ships = shipsHolder;
            add_ship_vertical(player, x,y+1,length-1);
            return 1;
        }
    }else{
        return 1;
    }
}