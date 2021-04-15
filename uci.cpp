#include "uci.h"


clock_t endt;
uint8_t endsd;

std::ofstream debugLog("./engine-log.txt");

void display()
{
    char c;
    for (uint64_t i = 1ULL << 56; i; )
    {
        c = '.'; //default is a point
        if (pawn_occ[0] & i) c = 'P';
        if (knight_occ[0] & i) c = 'N';
        if (bishop_occ[0] & i) c = 'B';
        if (rook_occ[0] & i) c = 'R';
        if (queen_occ[0] & i) c = 'Q';
        if (king_occ[0] & i) c = 'K';

        if (black_occ[0] & i) c |= 0x60; //make the letter lowercase if it's black
        std::cout << c << ' ';

        if (i & 0x8080808080808080) //if end of row reached
        {
            std::cout << "\n";
            i >>= 15;
        }
        else i <<= 1;
    }

    std::cout << (turn ? "Black" : "White") << " to move\nCastle rights: ";
    if (castle_rights[0] & 0x1) std::cout << "K";
    if (castle_rights[0] & 0x2) std::cout << "Q";
    if (castle_rights[0] & 0x4) std::cout << "k";
    if (castle_rights[0] & 0x8) std::cout << "q";
    if (ep[0])
    {
        std::cout << "\nEn passant possible on ";
        printf("%c%d", 'a' + (M42::lsb(ep[0]) % 8), 1 + (M42::lsb(ep[0]) >> 3));
    }
    std::cout << "\nHalf-move clock: " << 100 - hmc << " ply\nHash: " << std::hex << zhash[0] << std::dec << std::endl;
}

std::string str_move(Move move)
{
    std::string str;
    str += 'a' + (move.src % 8);
    str += '1' + (move.src >> 3);
    str += 'a' + (move.dst % 8);
    str += '1' + (move.dst >> 3);

    if (move.promo == KNIGHT_P) str += 'n';
    else if (move.promo == ROOK_P) str += 'r';
    else if (move.promo == BISHOP_P) str += 'b';
    else if (move.flags & PROMO_F) str += 'q';

    return str;
}

std::string str_eval(int64_t eval)
{
    std::string str;

    eval *= 1 - 2 * turn; //calculate score for white

    if (abs(eval) > MATE_DISP_THRESHOLD) //if there is checkmate
    {
        str = "mate ";
        if (eval > 0) //white winning
        {
            str += std::to_string(1 - eval - CHECKMATE >> 1);
        }
        else //black winning
        {
            str += std::to_string(CHECKMATE - eval >> 1);
        }
    }
    else
    {
        str = "cp ";
        str += std::to_string(eval);
    }

    return str;
}

void displayMove(Move move)
{
    std::cout << str_move(move);
}

void displayEvaluation(int64_t eval)
{
    std::cout << str_eval(eval);
}

bool input_available() {
    static bool init = false, is_pipe;
    static HANDLE stdin_h;
    DWORD val, error;

    if (!init) {

        init = true;

        stdin_h = GetStdHandle(STD_INPUT_HANDLE);

        is_pipe = !GetConsoleMode(stdin_h, &val);

        if (!is_pipe) {
            SetConsoleMode(stdin_h, val & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
            FlushConsoleInputBuffer(stdin_h);
        }
    }

    if (is_pipe) {

        if (!PeekNamedPipe(stdin_h, NULL, 0, NULL, &val, NULL)) {
            return true;
        }

        return val > 0;

    }
    else {

        GetNumberOfConsoleInputEvents(stdin_h, &val);
        return val > 1;
    }

    return false;
}

void uciLoop()
{
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    std::string cmd; //input buffer

    //initialize engine
    M42::init(); //initialize magics
    load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"); //load up starting position

    while (true)
    {
        fflush(stdout);
        std::getline(std::cin, cmd);

        debugLog << "Received command: " << cmd << std::endl;

        if (cmd == "") continue;
        else if (cmd == "isready") std::cout << "readyok\n"; //ready
        else if (cmd.substr(0, 10) == "ucinewgame") //begin a new game
        {
            load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
        }
        else if (cmd.substr(0, 8) == "position") parsePosition(cmd); //parse position
        else if (cmd.substr(0, 2) == "go") parseGo(cmd); //parse go command
        else if (cmd.substr(0, 5) == "perft") parsePerft(cmd); //parse perft command
        else if (cmd == "display") display();
        else if (cmd == "disp") display();
        else if (cmd == "d") display();
        else if (cmd == "quit") return;
        else if (cmd == "uci")
        {
            std::cout << "id name ACE v0.1.2\n";
            std::cout << "id author Checkmate6659 and DaYNASUS\n";

            std::cout << "option name Hash table size type spin default 256 min 1 max 4096\n";
            std::cout << "option name Enable Opening book type check default true\n";
            std::cout << "option name Book path type string default book.bin\n";

            std::cout << "uciok\n";
        }
        else if (cmd.substr(0, 9) == "setoption") parseOption(cmd);
        else std::cout << "Invalid command \"" << cmd << "\"\n";
    }
}

void parsePosition(std::string cmd)
{
    std::stringstream cmd_ss(cmd);
    std::string parsed;

    cmd_ss >> parsed;
    if (parsed != "position")
    {
        std::cout << "Invalid command \"" << cmd << "\"\n";
        return;
    }

    cmd_ss >> parsed;
    if (parsed == "startpos")
    {
        load_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
        cmd_ss >> parsed;
    }
    else if (parsed == "fen")
    {
        std::string total_fen = "";
        while (cmd_ss >> parsed)
        {
            if (parsed == "moves") break;
            total_fen += parsed + " ";
        }
        load_fen(total_fen);
    }
    else
    {
        std::cout << "Unsupported position format \"" << parsed << "\"\n";
        return;
    }
    //debug
    std::cout << "EVALUATION " << evaluate() << " MATERIAL " << countMaterial() << " PSQ " << computePSqTables(getGamePhase()) << " MAT. ADJ. " << calcMaterialAdjustments(getGamePhase()) << " RAW KSAFETY " << calcKingSafety() << " RAW PSHIELD " << calcPawnShields() << std::endl;
    debugLog << "EVALUATION " << evaluate() << " MATERIAL " << countMaterial() << " PSQ " << computePSqTables(getGamePhase()) << " MAT. ADJ. " << calcMaterialAdjustments(getGamePhase()) << " RAW KSAFETY " << calcKingSafety() << " RAW PSHIELD " << calcPawnShields() << std::endl;

    if (parsed != "moves") return;

    while (cmd_ss >> parsed) //move parser
    {
        MoveList moves = gen_moves();

        for (uint8_t i = moves.count; i;)
        {
            if (parsed == str_move(moves.moves[--i]))
            {
                make_move(moves.moves[i]);
                break;
            }
        }
    }

    define_as_root();
}

void parseGo(std::string cmd)
{
    std::stringstream cmd_ss(cmd);
    std::string parsed;

    cmd_ss >> parsed;
    if (parsed != "go")
    {
        std::cout << "Invalid command \"" << cmd << "\"\n";
        return;
    }

    uint8_t depth = MAX_SD;
    uint32_t time = 202000; //default time/inc are tuned for a move time of 5s with no params, default movestogo is 40
    uint32_t inc = 0;
    int32_t movetime = 0;
    uint8_t movestogo = 40;

    while (cmd_ss >> parsed)
    {
        if (parsed == "infinite")
        {
            movetime = 4294967295;
        }
        else if (parsed == "wtime" && !turn)
        {
            cmd_ss >> parsed;
            time = std::stoi(parsed);
        }
        else if (parsed == "btime" && turn)
        {
            cmd_ss >> parsed;
            time = std::stoi(parsed);
        }
        else if (parsed == "winc" && !turn)
        {
            cmd_ss >> parsed;
            inc = std::stoi(parsed);
        }
        else if (parsed == "binc" && turn)
        {
            cmd_ss >> parsed;
            inc = std::stoi(parsed);
        }
        else if (parsed == "depth")
        {
            cmd_ss >> parsed;
            depth = max(std::stoi(parsed), START_SD); //put a lower bound on the depth to avoid softlock
        }
        else if (parsed == "movetime")
        {
            cmd_ss >> parsed;
            movetime = std::stoi(parsed);
        }
        else if (parsed == "movestogo")
        {
            cmd_ss >> parsed;
            movestogo = std::stoi(parsed);
        }
    }

    endsd = MAX_SD;

    uint32_t allocTime = 4294967295; //default time is "infinite"
    if (movetime != 0) allocTime = movetime; //force allocated time to movetime
    else if (depth != MAX_SD) endsd = depth; //update maximal search depth
    else allocTime = allocatedTime(time, inc, movestogo); //normal mode: time manager handles execution

    negamaxRootIterDeepening(allocTime, new Move);
}

void parsePerft(std::string cmd)
{
    std::stringstream cmd_ss(cmd);
    std::string parsed;

    cmd_ss >> parsed;
    if (parsed != "perft")
    {
        std::cout << "Invalid command \"" << cmd << "\"\n";
        return;
    }

    cmd_ss >> parsed; //number

    int depth = atoi(parsed.c_str()) - 1; //there is a -1 because the function will be called for each move
    if (depth < 0)
    {
        std::cout << "Error: invalid depth\n";
        return;
    }

    uint64_t nodeCount = 0;
    MoveList moves = gen_moves();
    clock_t start = clock();

    for (int i = 0; i < moves.count; i++)
    {
        make_move(moves.moves[i]);
        uint64_t perft_result = perft(depth);
        displayMove(moves.moves[i]);
        printf(": %I64u\n", perft_result);
        nodeCount += perft_result;
        undo_move();
    }
    std::cout << "\nsearched " << nodeCount << " nodes in " << (uint64_t)(clock() - start) * 1000 / CLOCKS_PER_SEC << "ms\n";
}

void parseOption(std::string cmd)
{
    std::stringstream cmd_ss(cmd);
    std::string parsed;

    cmd_ss >> parsed;
    if (parsed != "setoption")
    {
        std::cout << "Invalid command \"" << cmd << "\"\n";
        return;
    }

    if (cmd.substr(0, 37) == "setoption name Hash table size value ")
    {
        debugLog << "CHANGE HASH '" << cmd.substr(37) << "'\n";
        //hashSize = NUM_TT_ENTRIES_1MB * stoi(cmd.substr(37)); //removed temp, as hash size is constant here
    }
    else if (cmd.substr(0, 42) == "setoption name Enable Opening book value ")
    {
        debugLog << "ENABLE OPENING BOOK -" << cmd.substr(42), "-\n";
    }
    else if (cmd.substr(0, 32) == "setoption name Book path value ")
    {
        debugLog << "CHANGE BOOK PATH -" << cmd.substr(32), "-\n";
    }
    else std::cout << "Invalid option \"" << cmd << "\"\n";

    return;
}
