#pragma once
#include "board_protocol.h"
#include <string>
#include <vector>
enum class BoardKind { Any,Chessnut,ChessLink };
struct Peer {std::string name,address; int type=0,rssi=0;BoardKind kind=BoardKind::Chessnut;};
struct BoardView {
  bool connected=false,busy=false,haveBoard=false;
  unsigned revision=0,dropped=0;
  core2::Board board{};
  BoardKind kind=BoardKind::Any;
  std::string status="Brett suchen";
  std::vector<Peer> peers;
};
void boardBegin();
BoardView boardView();
bool boardScan(BoardKind kind=BoardKind::Any,bool autoConnect=false);
bool boardConnect(int index);
void boardLeds(uint64_t mask);
