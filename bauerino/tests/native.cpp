#include "game_model.h"
#include "uci_session.h"
#include <iostream>
#include <stdexcept>
using namespace espmame;
void check(bool ok,const char* message) {if(!ok)throw std::runtime_error(message);}
void userMove(core2::Game& g,const char* text) {
  auto m=find_legal_move(g.pos,text);check(m!=0,"legal test move");
  Position next=g.pos;next.applyMove(m);g.board(core2::Game::diagram(next));
}
int main() {
  try {
    init_bitboards();init_zobrist();
    Position initial;initial.setFen(core2::Game::startFen);
    check(perft(initial,1)==20 && perft(initial,2)==400 && perft(initial,3)==8902,"start perft");
    core2::Game g;g.start(WHITE);g.board(core2::Game::diagram(g.pos));
    check(!g.setup && !g.needsEngine(),"white begins");
    auto lifted=core2::Game::diagram(g.pos);lifted[12]='.';g.board(lifted);
    check(g.moves.empty(),"lift is not a move");
    userMove(g,"e2e4");check(g.moves.size()==1 && g.needsEngine(),"player move triggers engine");
    g.thinking=true;check(!g.engineResult("e7e5",g.epoch+1),"old epoch rejected");
    check(g.engineResult("e7e5",g.epoch),"engine result accepted");
    check(g.moves.size()==1 && g.pending,"engine waits for physical execution");
    g.board(g.expected());check(g.moves.size()==2 && !g.pending,"execution commits engine move");
    check(g.positionCommand()=="position startpos moves e2e4 e7e5","complete UCI position");
    g.start(BLACK);g.board(core2::Game::diagram(g.pos));check(g.needsEngine() && g.human==BLACK,"engine starts white");
    g.board(core2::Game::diagram(g.pos));check(g.human==BLACK,"duplicate setup preserves chosen color");
    auto reverse=core2::Game::diagram(initial);std::reverse(reverse.begin(),reverse.end());
    g.start(BLACK);g.board(reverse);check(g.rotated && !g.setup,"rotated setup");
    g.start(WHITE);g.setup=false;g.pos.setFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");userMove(g,"e1g1");
    check(g.pos.pieceOn(SQ_F1)==make_piece(WHITE,ROOK) && g.pos.pieceOn(SQ_G1)==make_piece(WHITE,KING),"castling includes rook");
    g.start(WHITE);g.setup=false;g.pos.setFen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");userMove(g,"e5d6");
    check(g.pos.pieceOn(SQ_D5)==NO_PIECE,"en passant capture");
    g.start(WHITE);g.setup=false;g.pos.setFen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");userMove(g,"a7a8n");
    check(g.pos.pieceOn(SQ_A8)==make_piece(WHITE,KNIGHT),"underpromotion");
    uint8_t wire[34]={1,0x24};const std::string codes=".qkbpnRPrBNQK";
    auto board=core2::Game::diagram(initial);
    for(int i=0;i<64;i++)wire[2+i/2]|=uint8_t(codes.find(board[63-i]))<<(4*(i%2));
    core2::Board decoded{};check(core2::chessnutFrame(wire,34,decoded)&&decoded==board,"Chessnut nibble order");
    wire[2]=15;check(!core2::chessnutFrame(wire,34,decoded),"invalid piece rejected");
    auto leds=core2::chessnutLeds((1ULL<<SQ_A1)|(1ULL<<SQ_H8));check(leds[9]==128 && leds[2]==1,"LED mapping");
    check(core2::chesslinkName("MILLENNIUM CHESS") && core2::chesslinkName("ChessLink") && !core2::chesslinkName("Chessnut Air"),"ChessLink scan filter");
    auto mode=core2::modePosition(board);
    check(mode.size()==67 && core2::modeFrame(mode.data(),mode.size(),decoded) && decoded==board,"ChessLink parity/checksum and reversed file mapping");
    mode[8]^=1;check(!core2::modeFrame(mode.data(),mode.size(),decoded),"ChessLink corrupt frame rejected");
    auto modeLights=core2::modeLeds((1ULL<<SQ_A1)|(1ULL<<SQ_H8));
    check(modeLights.size()==167 && (modeLights[0]&127)=='L' && (modeLights[1]&127)=='0' && (modeLights[2]&127)=='1',"ChessLink full LED grid command");
    UciSession u;u.begin(0);u.line("option name Playing Style type combo default Very Solid var Very Solid var Fast Attack",1);
    u.line("option name Skill Level type spin default 5 min 1 max 10",1);
    u.line("option name Book File type string default <empty>",1);
    u.line("uciok",2);check(u.state==UciSession::ReadyWait,"wait for readyok");
    u.line("readyok",3);check(u.state==UciSession::Ready,"UCI ready");
    check(u.options[0].choices.size()==2 && u.options[0].value=="Very Solid","multiword option values");
    check(!u.set("Skill Level","11",4) && !u.set("Book File","bad\nquit",4),"range and injection validation");
    check(u.set("Playing Style","Fast Attack",4),"combo option");u.line("readyok",5);
    check(u.search("position startpos",7,6,100),"search starts");
    u.line("bestmove e2e4 ponder e7e5",7);check(u.bestmove=="e2e4" && u.generation==7,"bestmove and ponder");
    u.begin(10);u.tick(15011);check(u.state==UciSession::Failed,"handshake timeout");
    u.line("bestmove e2e4",15012);check(u.bestmove.empty(),"late result rejected");
    std::cout<<"PASS: rules, physical move reconciliation, colors, Chessnut, UCI lifecycle and options\n";
  } catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}
}
