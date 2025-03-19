#include "session.h"
#include "game.h"
#include <stdlib.h>
#include "antixss.h"
#include "userinput.h"

struct GlobalData global_data;

int callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED: 
            ((SessionData*)user)->id = global_data.current_id;
            AssignName16(((SessionData*)user)->username);
            ((SessionData*)user)->username_length = 16;
            ((SessionData*)user)->message_to_send = 0;
            ((SessionData*)user)->cards_count = 0;
            ((SessionData*)user)->is_playing = 0;
            ((SessionData*)user)->is_ready = 0;
            ((SessionData*)user)->is_privilege = 0;
            ((SessionData*)user)->is_active = 1;
            ((SessionData*)user)->profile = (rand()%8) + 1;

            printf("New Connection: [ \n");
            printf("        Id: %d\n", ((SessionData*)user)->id);
            printf("  Username: %s\n]\n", ((SessionData*)user)->username);
            global_data.connected_sessions_count++;
            global_data.current_id++;

            char message_id[6] = "i";
            strncpy(&message_id[1], (char*)(&((SessionData*)user)->id), 4);
            message_id[5] = 0;

            char message_w[28] = "M";
            strncpy(&message_w[1], ((SessionData*)user)->username, ((SessionData*)user)->username_length);
            memcpy(&message_w[((SessionData*)user)->username_length], " has joined", 12);
            SendTextToAllWs(wsi, message_w, 12 + ((SessionData*)user)->username_length);
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
                if(((SessionData*)user)->is_active) {
                    ((SessionData*)user)->is_active = 0;
                    lws_write(wsi, "T", 1, LWS_WRITE_TEXT);
                } else {
                    // remove connection
                }
                break;
            }
            lws_write(wsi, &((SessionData*)user)->message_to_send[LWS_PRE], 
                    ((SessionData*)user)->message_length-1, LWS_WRITE_TEXT);
                
            free(((SessionData*)user)->message_to_send);
            ((SessionData*)user)->message_to_send = 0;
            if(global_data.connected_sessions_to_send_data_count != 0) lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_RECEIVE:
            printf("id %d: ", ((SessionData*)user)->id);
            ManageUserInput(in, user, len, wsi);
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
