#ifndef uno_session
#define uno_session

#include <libwebsockets.h>

int callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len);
void SendTextToWs(struct lws *wsi, const char* text, size_t len, void *user);
void SendTextToAllWs(struct lws *wsi, const char* text, size_t len);
void SendTextToAllWsCon(struct lws_context* context, const struct lws_protocols* protocols, const char* text, size_t len);

struct Turn {
    int player_id;
    struct Turn* next_player;
    struct Turn* previous_player;
};

struct GlobalData {
    int current_id;
    int players_assigned;
    char table_card;
    char draw_count;
    
    char is_active_game;
    char is_playable_game;
    char is_flipped;
    char is_forced_game;
    char is_fresh_card;

    struct Turn* current_turn;
    struct Turn* first_turn;

    int ready_sessions_count;
    int connected_sessions_count;
    int connected_sessions_to_send_data_count;

    char* messages[16];
    size_t messages_length[16];
    char messages_pending;
    char last_message;
};

extern struct GlobalData global_data;

typedef struct {
    int id;
    char is_playing;
    char is_ready;
    char is_privilege;
    char is_active;

    char profile;
    char username[16];
    char username_length;
    char cards[64];
    char cards_count;
    char* message_to_send;
    size_t message_length;
} SessionData;

#endif