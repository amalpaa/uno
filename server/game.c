#include "game.h"
#include <stdlib.h>

void StartGame(struct lws* wsi) {
    global_data.is_active_game = 1;
    global_data.players_assigned = 0;
    char c[3];
    c[0] = 's';
    c[2] = 0;
    char b = 1;
    DrawCard(c, &b);
    global_data.table_card = c[1];

    printf("------- starting game\n");
    SendTextToAllWs(wsi, c, 3);
}

// For 16 chars
void AssignName16(char* name) {
    char firstchars = rand() % 10;
    char length = 0;

    switch (firstchars)
    {
    case 0:
        strcpy(name, "wiktorek");
        length = 8;
        break;
    case 1:
        strcpy(name, "dziabko");
        length = 7;
        break;
    case 2:
        strcpy(name, "gej");
        length = 3;
        break;
    case 3:
        strcpy(name, "c++ fan");
        length = 7;
        break;
    case 4:
        strcpy(name, "bohenek");
        length = 7;
        break;
    case 5:
        strcpy(name, "malpa");
        length = 5;
        break;
    case 6:
        strcpy(name, "komuch");
        length = 6;
        break;
    case 7:
        strcpy(name, "smerma");
        length = 6;
        break;
    case 8:
        strcpy(name, "turas");
        length = 5;
        break;
    case 9:
        strcpy(name, "bozydar");
        length = 7;
        break;
    }

    name[length] = ' ';
    name[length+1] = '#';
    for(int i=length+2; i<15; i++) {
        name[i] = '0'+(rand()%10);
    }
    name[15] = 0;
}

void GenerateStartingCards(char *cards, char* card_count)
{
    while(*card_count < 7){
        DrawCard(cards, card_count);
    }
}

void DrawCard(char *cards, char* card_count)
{
    cards[*card_count] = (rand()%88)+1;
    if(cards[*card_count] > 44) cards[*card_count] -= 44; 
    (*card_count)++;
}

void ConsoleCommand(void *in, struct lws *wsi, SessionData *user, size_t len) {
    if(len == 16) {
    if(strncmp(in, "m/tryb_babci: on", len) == 0) {
        ((SessionData*)user)->is_privilege = 1;
        printf("privileged mode on\n");
        SendTextToWs(wsi, "Mprivileged mode on", 20, user);
        return;

    }} else if(len == 17) {
    if(strncmp(in, "m/tryb_babci: off", len) == 0) {
        ((SessionData*)user)->is_privilege = 0;
        printf("privileged mode off\n");
        SendTextToWs(wsi, "Mprivileged mode off", 21, user);
        return;

    }} else if(len == 13) {
    if(strncmp(in, "m/force_reset", len) == 0) {
        if(!user->is_privilege) {
            printf("tried reseting game but not privileged\n");
            SendTextToWs(wsi, "MPermission denied", 19, user);
            return;
        }
        printf("reseting game\n");
        DestroyGame();
        SendTextToAllWs(wsi, "R", 2);
        return;

    }if(strncmp(in, "m/force_start", len) == 0) {
        if(!user->is_privilege) {
            printf("tried starting game but not privileged\n");
            SendTextToWs(wsi, "MPermission denied", 19, user);
            return;
        }
        printf("starting game\n");
        global_data.is_forced_game = 1;
        StartGame(wsi);
        return;
    }} else if(len == 12) {
    if(strncmp(in, "m/force_skip", len) == 0) {
        if(!user->is_privilege) {
            printf("tried skipping turn but not privileged\n");
            SendTextToWs(wsi, "MPermission denied", 19, user);
            return;
        }
        printf("skipping turn\n");
        ToNextPlayer();
        SendTextToAllWs(wsi, "d\0", 3);
        return;

    }} else if(len == 6) {
    if(strncmp(in, "m/help", len) == 0) {
        printf("used help\n");
        const char* message = "MAvailable commands:<br><span style=\"color:green\">PRIVILEGED</span><br>force_start - starts the game even if no all players are ready<br>force_reset - resets the game<br><span style=\"color:green\">NON PRIVILEGED</span><br>help - you just used it";
        SendTextToWs(wsi, message, 245, user);
        return;
    }}
    printf("unknown command\n");
    SendTextToWs(wsi, "MUnknown command; type /help", 29, user);
}

char CheckMove(const char card, const SessionData* user) {
    for(char i=0; i<user->cards_count; i++) {
        if(user->cards[i] == card) {
/////////////// when card is found
    if(global_data.table_card <= 44) {
        if(card <= 44) {
            if(global_data.table_card%4 == card%4 ||
               (char)((global_data.table_card-1) / 4) == (char)((card-1) / 4)) {
                return i+1;
            }
        }
    }
    return 0;
///////////////
        }
    }
    return 0;
}

char PlayMove(const char card_pos, SessionData* user, struct lws* wsi)
{
    if(user->cards[card_pos] >= 40 && user->cards[card_pos] < 45) {
        global_data.is_flipped = !global_data.is_flipped;
    }

    global_data.table_card = user->cards[card_pos];
    user->cards_count--;
    user->cards[card_pos] = user->cards[user->cards_count];

    char play[3];
    play[0] = 'w';
    play[1] = global_data.table_card;
    play[2] = 0;

    if(user->cards_count == 0) {
        play[0] = 'W';
        SendTextToAllWs(wsi, play, sizeof(play));
        DestroyGame();
        return 1;
    }
    SendTextToAllWs(wsi, play, sizeof(play));
    return 0;
}

const char *CardToString(const char card)
{
    switch (card)
    {
    case 1: return "r0";
    case 5: return "r1";
    case 9: return "r2";
    case 13: return "r3";
    case 17: return "r4";
    case 21: return "r5";
    case 25: return "r6";
    case 29: return "r7";
    case 33: return "r8";
    case 37: return "r9";
    case 41: return "rr";

    case 2: return "g0";
    case 6: return "g1";
    case 10: return "g2";
    case 14: return "g3";
    case 18: return "g4";
    case 22: return "g5";
    case 26: return "g6";
    case 30: return "g7";
    case 34: return "g8";
    case 38: return "g9";
    case 42: return "gr";

    case 3: return "b0";
    case 7: return "b1";
    case 11: return "b2";
    case 15: return "b3";
    case 19: return "b4";
    case 23: return "b5";
    case 27: return "b6";
    case 31: return "b7";
    case 35: return "b8";
    case 39: return "b9";
    case 43: return "br";

    case 4: return "y0";
    case 8: return "y1";
    case 12: return "y2";
    case 16: return "y3";
    case 20: return "y4";
    case 24: return "y5";
    case 28: return "y6";
    case 32: return "y7";
    case 36: return "y8";
    case 40: return "y9";
    case 44: return "yr";

    default: return "whatthehell";
    }
}

void DestroyGame() {
    struct Turn* temp;
    for(;global_data.players_assigned != 0; global_data.players_assigned--) {
        temp = global_data.first_turn->next_player;
        free(global_data.first_turn);
        global_data.first_turn = temp;
    }

    global_data.is_playable_game = 0;
    global_data.is_active_game = 0;
    global_data.ready_sessions_count = 0;
    global_data.current_turn = 0;
    global_data.is_flipped = 0;
}

void ToNextPlayer() {
    if(global_data.is_flipped) global_data.current_turn = global_data.current_turn->previous_player;
    else global_data.current_turn = global_data.current_turn->next_player;
}