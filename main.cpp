#include <iostream>
#include <time.h>

#define MINUTE 60000
#define SECOND 1000

//#define PERFT_DEBUG

#include "board.h"
#include "search.h"
#include "uci.h"


int main(int argc, char** argv)
{
    uciLoop();

    //Perft positions: https://www.chessprogramming.org/Perft_Results
    //KIWIPETE (POS 2): r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -
    //CPW POS 3 (useful for ep): 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -
    //CPW POS 4: r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -
    //MIRRORED 4: r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -
    //CPW POS 5: rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -
    //CPW POS 6: r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - -
    // ------------------
    // Other test positions:
    //Checkmate in 5 moves with Queen sac (white): 5rk1/5ppp/3r4/6N1/2Q5/8/1R2BPPP/6K1 w - -
    //Same but mirrored: 6k1/1r2bppp/8/2q5/6n1/3R4/5PPP/5RK1 b - -
    //For killer heuristics: Silent but deadly position 1 (best move: Qc7) 1qr3k1/p2nbppp/bp2p3/3p4/3P4/1P2PNP1/P2Q1PBP/1N2R1K1 b - -
    //From my game: r1b2rk1/p1ppR3/1p2p3/5pN1/5P2/4P3/PPPP4/RNB1KB2 w Q - (random position; White ez win)

    //test PSQ: rnbqkbnr/pppppppp/8/8/2BPPB2/2N2N2/PPP2PPP/R2QK2R w KQkq - 0 1
    //Mate in 3: position fen 5rk1/5ppp/3r4/6N1/2Q5/8/1R2BPPP/6K1 w - - 0 1 moves c4f7 f8f7 b2b8 d6d8

    //Lasker-Reichhelm position: 8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - -
    //King/2 Rooks endgame: 8/8/1k6/8/6R1/8/3KR3/8 w - -
    //King/1 Rook endgame: 8/8/1k6/8/6R1/8/3K4/8 w - -

    clock_t start = clock();
    for (int i = 0; i < 1000000; i++) evaluate();
    std::cout << clock() - start;

    return 0;
}
