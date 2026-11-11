#pragma once
#ifdef BAUERINO_DISPLAY
#include "game_model.h"
#include "board_link.h"
#include "bauerino_search.h"
enum class DisplayAction { PlayVirginia, PlayRetro, Preview, BotPreview, NewGame, NewBot, Abort, Scan, Connect };
int displayCurrentBot();
void displayAction(DisplayAction action, int first=0, int second=0, char promotion=0);
bool displayBegin();
bool displayDiagnostic(const std::string& command);
void displayTick(const core2::Game& game, const BoardView& board, bool active,
                 bool touchInput, const std::string& error,const bauerino::SearchView& search);
#endif
