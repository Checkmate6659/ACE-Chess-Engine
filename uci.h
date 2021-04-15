#pragma once

#include <iostream>
#include <sstream>

#include <fstream> //DEBUG

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <windows.h>

#include "board.h"
#include "search.h"
#include "timeManager.h"
#include "tt.h"

#define MATE_DISP_THRESHOLD 4611686018427387648


extern clock_t endt;
extern uint8_t endsd;

void display();

std::string str_move(Move move);
std::string str_eval(int64_t eval);

void displayMove(Move move);
void displayEvaluation(int64_t eval);

bool input_available();

void uciLoop();
void parsePosition(std::string cmd);
void parseGo(std::string cmd);
void parsePerft(std::string cmd);
void parseOption(std::string cmd);
