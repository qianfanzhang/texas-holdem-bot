#ifndef _ADAPTER_H
#define _ADAPTER_H

#include <utility>
#include <string>
#include <cinttypes>

// Return a (type, bet) pair.
// See definition of ActionType in ../game.h
// WARNING: player=0: bigblind, player=1: smallblind.
std::pair<int, int> getAction(int round, int player, int* spent, int raiseLimit, uint8_t* holeCards, uint8_t* boardCards, std::string bp_path);

#endif
