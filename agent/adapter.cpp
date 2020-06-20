#include "adapter.h"
#include "cards.h"
#include "game.h"
#include "blueprint.h"

static Blueprint bp;
static bool initialized = false;

std::pair<int, int> getAction(int round, int player, int* spent, int raiseLimit, uint8_t* holeCards, uint8_t* boardCards, std::string bp_path, std::string abs_dir, bool* raised, bool always_fold) {
    if (!initialized) {
        bp.load(bp_path);
        initAbstraction(abs_dir, 1000, 2000);
        initialized = true;
    }

    const int num_boardcards[4] = {0, 3, 4, 5};
    GameState g;
    assert(round >= 0 && round < 4);
    int max_spent = std::max(spent[0], spent[1]);

    g.round = round;
    g.player = player ^ 1;
    g.spent[0] = spent[1];
    g.spent[1] = spent[0];
    g.last_raise = raiseLimit - max_spent;
    g.cards.holeCards[player ^ 1] = getMask(holeCards, 2);
    g.cards.boardCards = getMask(boardCards, num_boardcards[round]);
    g.raised[0] = raised[1];
    g.raised[1] = raised[0];

    assert(g.hasAction());

    Action action = bp.sampleAction(g, true, always_fold);
    if (action.type == FOLD)
        return std::make_pair(0, 0);
    if (action.type == CALL)
        return std::make_pair(1, 0);
    if (action.type == ALLIN)
        return std::make_pair(2, STACK_SIZE);
    if (action.type == RAISE)
        return std::make_pair(2, max_spent + action.bet);
    abort();
}
