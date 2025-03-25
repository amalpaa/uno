#include "userinput.h"
#include "stdio.h"
#include "session.h"
#include "game.h"
#include "antixss.h"

void ManageUserInput(char* in, SessionData* user, size_t len, struct lws* wsi) {
    switch (*in) {
    case _PING_PONG_TEST:
        user->is_active = 1;
        break;

    case _READY_UNREADY: 
        if(global_data.is_active_game) {
            printf("tried changing ready state but the game already started\n");
            break;
        }
        if(user->is_playing) {
            user->is_ready = 0;
            user->is_playing = 0;
        } 
        if(!user->is_ready) {
            printf("changed state to ready\n");
            user->is_ready = 1;
            global_data.ready_sessions_count++; 
            if(global_data.connected_sessions_count > 1 && 
                global_data.ready_sessions_count == global_data.connected_sessions_count) {
                StartGame(wsi);
            }
        } 
        else {
            printf("changed state to unready\n");
            user->is_ready = 0;
            global_data.ready_sessions_count--;
        }
        break;

    case _CHANGING_USERNAME:
        if(len < 6) {
            printf("tried changing username but was too short\n");
            break;
        } else if(len > 16) {
            printf("tried changing username but was too long\n");
            break;
        }   
        user->username_length = len;
        user->username[len-1] = 0;
        strncpy(user->username, &in[1], len-1);
        printf("changed username to %s\n", user->username); 
        if(user->is_playing && global_data.is_playable_game) {
            char message[24];
            message[0] = 'q';
            strncpy(&message[1], (char*)&user->id, 4);
            strncpy(&message[5], user->username, len-1);
            message[len+4] = 0;
            SendTextToAllWs(wsi, message, 5+len);
        }
        break;

    case _SENDING_MESSAGE:
        char message[256] = "m";
        if(len == 1) {
            printf("empty message\n");
            break;
        }
        if(in[1] == '/') {
            ConsoleCommand(in, wsi, user, len);
            break;
        }
        if(!isSafe(&in[1])) break;
        if(len > 200) { 
            printf("tried sending message but is too long\n");
            break;
        }   
        message[1] = user->username_length;
        printf("send message \"%.*s\"\n", (int)len-1, &in[1]);
        strcpy(&message[2], user->username);
        SendTextToAllWs(wsi, strncat(message, &in[1], len-1), len+user->username_length+1);
        break;

    case _DRAWING_CARD:
        if(!global_data.is_playable_game) {
            printf("tried to draw a card but the game has not started\n");
            break;
        }
        if(global_data.current_turn->player_id!=user->id) {
            printf("tried to draw a card but was not his turn\n");
            break;
        }
        char message_card[64];
        message_card[0] = 'u';
        message_card[1] = global_data.draw_count;

        char draw_message[3];
        draw_message[0] = 'd';
        draw_message[1] = global_data.draw_count;
        draw_message[2] = 0;

        printf("draw card: "); 
        for(int i=2; i<global_data.draw_count+2; i++) {
            DrawCard(user->cards, &user->cards_count);
            message_card[i] = user->cards[user->cards_count-1];
            printf("%s ", CardToString(message_card[i]));
        }
        message_card[2+global_data.draw_count] = 0;
        printf("\n");
        ToNextPlayer();
        SendTextToWs(wsi, message_card, 3+global_data.draw_count, user);
        SendTextToAllWs(wsi, draw_message, 3);

        global_data.draw_count = 1;
        global_data.is_fresh_card = 0;
        break;

    case _JOINING_GAME:
        if((!user->is_ready || global_data.is_playable_game) && !global_data.is_forced_game) {
            printf("tried joining but game already started\n");
            break;
        } else if (user->is_playing && !global_data.is_forced_game) {
            printf("tried joining but already joined\n");
            break;
        }
        user->cards_count = 0;
        user->is_playing = 1;

        // Generate starting card and send them to the user
        GenerateStartingCards(user->cards, &(user->cards_count));
        char first_cards[17];
        first_cards[0] = 'p';
        first_cards[16] = 0;
        strncpy(&first_cards[1], user->cards, 7);
        strncpy(&first_cards[8], (char*)(&global_data.players_assigned), 4);    

        if(!global_data.is_forced_game) strncpy(&first_cards[12], (char*)(&global_data.ready_sessions_count), 4);
        else strncpy(&first_cards[12], (char*)(&global_data.connected_sessions_count), 4);
        SendTextToWs(wsi, first_cards, sizeof(first_cards), user);  

        // Add player to turn list
        struct Turn* this_player = malloc(sizeof(struct Turn));
        if(global_data.current_turn == 0) { // First Connected Player
            global_data.first_turn = this_player;
        } else { // Players in the middle
            this_player->previous_player = global_data.current_turn;
            global_data.current_turn->next_player = this_player;
        } // All Players
        global_data.current_turn = this_player; 
        global_data.current_turn->player_id = user->id;
        global_data.players_assigned++; 

        // Send information about new user to users
        char connected_user_message[23];
        connected_user_message[0] = 'c';
        strncpy(&connected_user_message[1], (char*)(&user->id), 4);
        strncpy(&connected_user_message[5], user->username, 16);
        connected_user_message[21] = user->profile;
        connected_user_message[22] = 0;
        SendTextToAllWs(wsi, connected_user_message, sizeof(connected_user_message));

        printf("joined the game with cards: ");  
        for(int i=1; i<8; i++) printf("%s ", CardToString(first_cards[i]));
        printf("\n");   

        // Close turn list and start game if is the last player
        if((global_data.players_assigned == global_data.ready_sessions_count && !global_data.is_forced_game) ||
           (global_data.is_forced_game && global_data.players_assigned == global_data.connected_sessions_count)) {
            printf("------- all players joined; game started\n");
            global_data.first_turn->previous_player = global_data.current_turn;
            global_data.current_turn->next_player = global_data.first_turn;
            global_data.current_turn = global_data.first_turn;
            global_data.is_playable_game = 1;
            global_data.is_forced_game = 0;
            SendTextToAllWs(wsi, "P", 2);   
        }   
        break;

    case _PLAYING_MOVE:
        if(!global_data.is_playable_game) {
            printf("tried to play a move but the game has not started\n");
            break;
        }
        if(len < 2) {
            printf("tried to play a move but sent invalid packet size\n");
            break;
        }
        if(global_data.current_turn->player_id!=user->id) {
            printf("tried to play a move but was not his turn\n");
            break;
        }

        char card_pos = CheckMove(in[1]+1, user);
        if(!card_pos) {
            printf("tried to play a move but was invalid\n");
            CheckMove(in[1]+1, user);
            break;
        }
        if(PlayMove(card_pos-1, user, wsi)) {
            printf("played last card; won the game\n");
            break;
        }
        printf("played card %s\n", CardToString(global_data.table_card));
        ToNextPlayer();
        break;

    default:
        printf("unknown\n");
        break;
    }
}