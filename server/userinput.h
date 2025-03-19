#ifndef uno_userinput
#define uno_userinput

#include "session.h"

void ManageUserInput(char* in, SessionData* user, size_t len, struct lws* wsi);

enum UserMessages {
    _PING_PONG_TEST     = 'T',
    _READY_UNREADY      = 'r',
    _CHANGING_USERNAME  = 'q',
    _SENDING_MESSAGE    = 'm',
    _DRAWING_CARD       = 'd',
    _JOINING_GAME       = 's',
    _PLAYING_MOVE       = 'p'
};

#endif