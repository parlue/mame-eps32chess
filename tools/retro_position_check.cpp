// Host-only legality check, using the product's actual chess move generator.
#include "position.h"
#include "movegen.h"
#include <iostream>
#include <string>
int main() {
    init_bitboards(); init_zobrist();
    Position position;
    if(!position.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"))return 1;
    std::string token; unsigned plies=0;
    while(std::cin >> token) {
        Move move=find_legal_move(position,token);
        if(!move){std::cerr << "Illegal move at ply " << plies+1 << ": " << token << '\n';return 2;}
        position.applyMove(move);++plies;
        if(!position.sanityCheck())return 3;
    }
    MoveList legal;generateLegal(position,legal);
    std::cout << "{\"fen\":\"" << position.fen() << "\",\"plies\":" << plies << ",\"legal\":[";
    for(int i=0;i<legal.count;++i)std::cout << (i?",":"") << '"' << move_to_uci(legal[i]) << '"';
    std::cout << "]}\n";
}
