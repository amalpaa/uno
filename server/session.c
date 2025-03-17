#include "session.h"
#include "game.h"
#include <stdlib.h>
#include "antixss.h"

struct GlobalData global_data;

int callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: 
            ((SessionData*)user)->id = global_data.current_id;
            AssignName16(((SessionData*)user)->username);
            ((SessionData*)user)->message_to_send = 0;
            ((SessionData*)user)->cards_count = 0;
            ((SessionData*)user)->is_playing = 0;
            ((SessionData*)user)->is_ready = 0;
            ((SessionData*)user)->is_privilege = 0;
            ((SessionData*)user)->profile = (rand()%8) + 1;

            printf("New Connection: [ \n");
            printf("        Id: %d\n", ((SessionData*)user)->id);
            printf("  Username: %s\n]\n", ((SessionData*)user)->username);
            global_data.connected_sessions_count++;
            global_data.current_id++;

            char message_w[28] = "M";
            char message_id[6] = "i";

            strncpy(&message_id[1], (char*)(&((SessionData*)user)->id), 4);
            strncpy(&message_w[1], ((SessionData*)user)->username, 16);
            message_id[5] = 0;

            SendTextToAllWs(wsi, strcat(message_w, " has joined"), 28);
            SendTextToWs(wsi, message_id, sizeof(message_id), user);
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE: 
            if(((SessionData*)user)->message_to_send==0) {
                if(global_data.connected_sessions_to_send_data_count != 0) {
                    global_data.connected_sessions_to_send_data_count--;
                    char _message_pos = global_data.last_message-global_data.messages_pending;
                    if(_message_pos < 0) _message_pos += sizeof(global_data.messages)/sizeof(char*);

                    lws_write(wsi, (global_data.messages[_message_pos] + LWS_PRE), 
                        global_data.messages_length[_message_pos]-1, LWS_WRITE_TEXT);

                    if(global_data.connected_sessions_to_send_data_count == 0) {
                        global_data.messages_pending--;
                        free(global_data.messages[_message_pos]);
                        if(global_data.messages_pending > 0) {
                            global_data.connected_sessions_to_send_data_count = global_data.connected_sessions_count;
                            lws_callback_on_writable_all_protocol(lws_get_context(wsi), lws_get_protocol(wsi));
                        }
                    }
                    break;
                }
                printf("write operation called with empty buffer (not good)\n");
                break;
            }
            lws_write(wsi, &((SessionData*)user)->message_to_send[LWS_PRE], 
                    ((SessionData*)user)->message_length-1, LWS_WRITE_TEXT);
                
            free(((SessionData*)user)->message_to_send);
            ((SessionData*)user)->message_to_send = 0;
            if(global_data.connected_sessions_to_send_data_count != 0) lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_RECEIVE:
            printf("message from id %d: ", ((SessionData*)user)->id);
            switch (*(char*)in)
            {
                      /////////////////////////
            case 'r': //        Ready / unready
                if(global_data.is_active_game) {
                    printf("tried changing ready state but the game already started\n");
                    break;
                }
                if(((SessionData*)user)->is_playing) {
                    ((SessionData*)user)->is_ready = 0;
                    ((SessionData*)user)->is_playing = 0;
                } 
                if(!((SessionData*)user)->is_ready) {
                    printf("changed state to ready\n");
                    ((SessionData*)user)->is_ready = 1;
                    global_data.ready_sessions_count++;

                    if(global_data.connected_sessions_count > 1 && 
                        global_data.ready_sessions_count == global_data.connected_sessions_count) {
                        StartGame(wsi);
                    }
                } 
                else {
                    printf("changed state to unready\n");
                    ((SessionData*)user)->is_ready = 0;
                    global_data.ready_sessions_count--;
                }
                break;
                      /////////////////////////
            case 'm': //        Sending message
                char message[256] = "m";
                if(len == 1) {
                    printf("empty message");
                    break;
                }
                if(((char*)in)[1] == '/') {
                    ConsoleCommand(in, wsi, user, len);
                    break;
                }
                if(isSafe(&((char*)in)[1])) break;
                if(len > 200) { 
                    printf("tried sending message but is too long\n");
                    break;
                } 

                printf("send message \"%.*s\"\n", (int)len-1, &((char*)in)[1]);
                strcpy(&message[1], ((SessionData*)user)->username);
                SendTextToAllWs(wsi, strncat(message, &((char*)in)[1], len-1), len+16);
                break;
            case 'd':
                if(!global_data.is_playable_game) {
                    printf("tried to draw a card but the game has not started\n");
                    break;
                }
                if(global_data.current_turn->player_id!=((SessionData*)user)->id) {
                    printf("tried to draw a card but was not his turn\n");
                    break;
                }
                DrawCard(((SessionData*)user)->cards, &((SessionData*)user)->cards_count);
                char message_card[4];
                message_card[0] = 'u';
                message_card[1] = 1;
                message_card[2] = ((SessionData*)user)->cards[((SessionData*)user)->cards_count-1];
                message_card[3] = 0;
                printf("draw card: %s\n", CardToString(message_card[2])); 
                ToNextPlayer();
                SendTextToWs(wsi, message_card, sizeof(message_card), user);
                SendTextToAllWs(wsi, "d\1", 3);
                break;
                      //////////////////////
            case 's': //        Joining game
                if((!((SessionData*)user)->is_ready || global_data.is_playable_game) && !global_data.is_forced_game) {
                    printf("tried joining but game already started\n");
                    break;
                } else if (((SessionData*)user)->is_playing && !global_data.is_forced_game) {
                    printf("tried joining but already joined\n");
                    break;
                }
                ((SessionData*)user)->cards_count = 0;
                ((SessionData*)user)->is_playing = 1;
                GenerateStartingCards(((SessionData*)user)->cards, &(((SessionData*)user)->cards_count));
                char first_cards[17];
                first_cards[0] = 'p';
                first_cards[16] = 0;
                strncpy(&first_cards[1], ((SessionData*)user)->cards, 7);
                strncpy(&first_cards[8], (char*)(&global_data.players_assigned), 4);

                if(!global_data.is_forced_game) strncpy(&first_cards[12], (char*)(&global_data.ready_sessions_count), 4);
                else strncpy(&first_cards[12], (char*)(&global_data.connected_sessions_count), 4);
                SendTextToWs(wsi, first_cards, sizeof(first_cards), user);

                struct Turn* this_player = malloc(sizeof(struct Turn));
                if(global_data.current_turn == 0) { // First Connected Player
                    global_data.first_turn = this_player;
                } else { // Players in the middle
                    this_player->previous_player = global_data.current_turn;
                    global_data.current_turn->next_player = this_player;
                } // All Players
                global_data.current_turn = this_player; 
                global_data.current_turn->player_id = ((SessionData*)user)->id;
                global_data.players_assigned++;

                char connected_user_message[23];
                connected_user_message[0] = 'c';
                strncpy(&connected_user_message[1], (char*)(&((SessionData*)user)->id), 4);
                strncpy(&connected_user_message[5], ((SessionData*)user)->username, 16);
                connected_user_message[21] = ((SessionData*)user)->profile;
                connected_user_message[22] = 0;
                SendTextToAllWs(wsi, connected_user_message, sizeof(connected_user_message));

                printf("joined the game with cards: ");  
                for(int i=1; i<8; i++) printf("%s ", CardToString(first_cards[i]));
                printf("\n");

                if((global_data.players_assigned == global_data.ready_sessions_count && !global_data.is_forced_game) ||
                   (global_data.is_forced_game && global_data.players_assigned == global_data.connected_sessions_count)) { // Last Player
                    printf("------- all players joined; game started\n");
                    global_data.first_turn->previous_player = global_data.current_turn;
                    global_data.current_turn->next_player = global_data.first_turn;
                    global_data.current_turn = global_data.first_turn;
                    global_data.is_playable_game = 1;
                    global_data.is_forced_game = 0;
                    SendTextToAllWs(wsi, "P", 2);

                }

                break;
                      ////////////////////////
            case 'p': //        Playing a move
                if(!global_data.is_playable_game) {
                    printf("tried to play a move but the game has not started\n");
                    break;
                }
                if(len < 2) {
                    printf("tried to play a move but sent invalid packet size\n");
                    break;
                }
                if(global_data.current_turn->player_id!=((SessionData*)user)->id) {
                    printf("tried to play a move but was not his turn\n");
                    break;
                }
                char card_pos = CheckMove(((char*)in)[1]+1, user);
                if(!card_pos) {
                    printf("tried to play a move but was invalid\n");
                    CheckMove(((char*)in)[1]+1, user);
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
            break;
        case LWS_CALLBACK_CLOSED:
            global_data.connected_sessions_count--;
            printf("user has disconnected id: %d\n", ((SessionData*)user)->id);
            if(((SessionData*)user)->message_to_send!=0) free(((SessionData*)user)->message_to_send);
            if(global_data.is_playable_game) {
                if(((SessionData*)user)->is_playing) {
                    global_data.players_assigned--;
                }
            }
            char message[26] = "M";
            if(global_data.connected_sessions_count != 0) {
                strcpy(&message[1], ((SessionData*)user)->username);
                SendTextToAllWs(wsi, strcat(message, " has left"), 26);
            }
            break;
    }
    return 0;
}

void SendTextToWs(struct lws *wsi, const char *text, size_t len, void *user)
{
    if(((SessionData*)user)->message_to_send != 0) {
        printf("message already stored (very bad)\n");
        return;
    }
    ((SessionData*)user)->message_length = len;
    ((SessionData*)user)->message_to_send = malloc(LWS_PRE + len);
    memmove(&((SessionData*)user)->message_to_send[LWS_PRE], text, len);
    lws_callback_on_writable(wsi);
}

void SendTextToAllWs(struct lws *wsi, const char *text, size_t len)
{
    SendTextToAllWsCon(lws_get_context(wsi), lws_get_protocol(wsi), text, len);
}

void SendTextToAllWsCon(struct lws_context* context, const struct lws_protocols* protocols, const char *text, size_t len)
{
    global_data.messages_length[global_data.last_message] = len;
    global_data.messages[global_data.last_message] = malloc(LWS_PRE + len);
    memmove(global_data.messages[global_data.last_message] + LWS_PRE, text, len);
    global_data.last_message++;
    if(global_data.last_message >= sizeof(global_data.messages)/sizeof(char*)) global_data.last_message = 0;
    if(global_data.messages_pending == 0) {
        global_data.connected_sessions_to_send_data_count = global_data.connected_sessions_count;
        lws_callback_on_writable_all_protocol(context, protocols);
    }
    global_data.messages_pending++;
}
