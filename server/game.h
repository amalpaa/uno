#ifndef uno_game
#define uno_game

#include "session.h"

void StartGame(struct lws* wsi);
void AssignName16(char* name);
void GenerateStartingCards(char* cards, char* card_count);
void DrawCard(char* cards, char* card_count);
void ConsoleCommand(void* in, struct lws* wsi, SessionData* user, size_t len);
void DestroyGame();
char CheckMove(const char card, const SessionData* user);
char PlayMove(const char card_pos, SessionData* user, struct lws* wsi);
const char* CardToString(const char card);
void ToNextPlayer();

#endif