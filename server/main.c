#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "session.h"
#include "useful.h"

int main(int argc, char **argv) {

    int port;
    if (argc == 2) {
        port = usef_CharToInt(argv[1]);
    } else if(argc == 1) {
        port = 8080;
    } else {
        printf("Invalid argument amount\n");
        return -1;
    }
    
    static struct lws_protocols protocols[] = {
        {
            "uno-protocol", 
            callback, 
            sizeof(SessionData), 
            0, 
            0, 0, 0
        },
        { 0, 0, 0, 0 } 
    };

    struct lws_context_creation_info info = {
        .port = port, 
        .protocols = protocols 
    };
    struct lws_context *context = lws_create_context(&info);
    srand(time(0));
    
    if (!context) {
        printf("Failed to create WebSocket context.\n");
        return -1;
    }

    global_data.ready_sessions_count = 0;
    global_data.current_id = 0;
    global_data.is_active_game = 0;
    global_data.is_playable_game = 0;
    global_data.connected_sessions_count = 0;
    global_data.connected_sessions_to_send_data_count = 0;
    global_data.messages_pending = 0;
    global_data.last_message = 0;
    global_data.is_flipped = 0;
    global_data.is_forced_game = 0;

    printf("Websocket context created. Entering main loop.\n");
    while (1) { 
        lws_service(context, 0);
    }
    lws_context_destroy(context);
    return 0;
}