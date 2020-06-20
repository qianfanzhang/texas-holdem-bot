#ifndef _GAME_H
#define _GAME_H

#include <vector>
#include <utility>
#include <cassert>
#include "cards.h"

const int STACK_SIZE = 20000;
const int SMALL_BLIND = 50;
const int BIG_BLIND = 100;

enum ActionType { FOLD, CALL, ALLIN, RAISE };

struct Action {
    ActionType type;            // 0:fold, 1:call, 2:allin, 3:raise
    int bet;
};

struct GameState {
    int round;                  // 0:preflop, 1:flop, 2:turn, 3:river, 4: end
    int player;                 // 0:smallblind, 1:bigblind, -1:chance
    bool fold, allin;
    bool first_player;
    CardState cards;
    int spent[2];
    int last_raise;
    bool raised[2];

    GameState() {
        round = 0;
        player = -1;
        first_player = true;
        fold = allin = false;
        spent[0] = SMALL_BLIND;
        spent[1] = BIG_BLIND;
        last_raise = BIG_BLIND;
        raised[0] = raised[1] = false;
    }

    int pot() const {
        return spent[0] + spent[1];
    }

    bool isTerminal() const {
        return round == 4 || fold;
    }

    bool isChance() const {
        return !isTerminal() && player == -1;
    }

    bool hasAction() const {
        return player != -1 && !allin;
    }

    int maxSpent() const {
        return std::max(spent[0], spent[1]);
    }

    int minRaise() const {
        assert(canRaise());
        return last_raise;
    }

    int maxRaise() const {
        assert(canRaise());
        return STACK_SIZE - maxSpent() - 1;
    }

    bool canFold() const {
        return player != -1 && spent[player] < spent[player ^ 1];
    }

    bool canRaise() const {
        return player != -1 && maxSpent() + last_raise < STACK_SIZE;
    }

    bool canAllin() const {
        return player != -1 && maxSpent() < STACK_SIZE;
    }

    void print() const;

    int getValue() const;
    void sampleCard();
    bool doAction(Action action); // false: next round; true: continue
};

#endif
