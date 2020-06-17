#include "game.h"

void GameState::print() const {
    printf("round=%d, player=%d, fold=%d, allin=%d, first_player=%d, last_raise=%d\n", round, player, fold, allin, first_player, last_raise);
    cards.print();
    printf("pot: %d, %d\n", spent[0], spent[1]);
}

int GameState::getValue() const {
    assert(isTerminal());
    if (fold) {                 // someone folded
        // assert(spent[0] != spent[1]);
        return spent[0] > spent[1] ? spent[1] : -spent[0];
    }
    assert(spent[0] == spent[1]);
    int v0 = cards.getHandValue(0);
    int v1 = cards.getHandValue(1);
    if (v0 != v1)
        return v0 > v1 ? spent[1] : -spent[0];
    return 0;
}

void GameState::sampleCard() {
    assert(isChance());
    if (round == 0) {
        cards.sampleHoleCard(0, 2);
        cards.sampleHoleCard(1, 2);
        player = 0;             // smallblind first
    } else {
        int num_cards = (round == 1 ? 3 : 1);
        cards.sampleBoardCard(num_cards);
        player = 1;             // bigblind first
    }
    first_player = true;
    last_raise = BIG_BLIND;

    if (allin) {
        assert(!fold);
        player = -1;
        ++round;
    }
}

bool GameState::doAction(Action action) {
    assert(hasAction());
    assert(spent[player] <= spent[player ^ 1]);

    if (action.type == FOLD) {
        assert(canFold());
        fold = true;
        player = -1;
        ++round;
    } else if ((action.type == FOLD && spent[player] == spent[player ^ 1]) || action.type == CALL) {
        spent[player] = spent[player ^ 1];
        if (first_player) {
            assert(!allin);
            first_player = false;
            player ^= 1;
        } else {
            if (spent[player] == STACK_SIZE) // all-in
                allin = true;
            player = -1;
            ++round;
        }
    } else if (action.type == ALLIN) {
        assert(spent[player] < STACK_SIZE);
        spent[player] = STACK_SIZE;
        last_raise = STACK_SIZE; // the other player must fold or call all-in
        first_player = false;
        player ^= 1;
    } else {
        assert(action.type == RAISE);

        assert(action.bet >= minRaise() && action.bet <= maxRaise());
        spent[player] = maxSpent() + action.bet;
        last_raise = action.bet;
        first_player = false;
        player ^= 1;
    }
    return player != -1;
}
