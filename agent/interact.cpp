#include <string>
#include "blueprint.h"
#include "game.h"
#include "abstraction.h"
#include "random.h"

Blueprint bp;

int input(std::string prompt) {
    printf("%s: ", prompt.c_str());
    int x;
    scanf("%d", &x);
    return x;
}

int main() {
    bp.load("blueprint_checkpoint.txt");

    GameState g;
    int human = input("Player");
    for (;;) {
        g.print();
        printf("\n");

        if (g.isTerminal()) {
            printf("Game End!\nUtility = %d\n", human == 0 ? g.getValue() : -g.getValue());
            break;
        }
        if (g.isChance()) {
            g.sampleCard();
        } else if (g.player == human) {
            ActionType type = ActionType(input("Action type (FOLD, CALL, ALLIN, RAISE)"));
            int bet = 0;
            if (type == RAISE)
                bet = input("Raise size");
            g.doAction(Action({type, bet}));
        } else {
            assert(g.player == (human ^ 1));
            int infoset = getGameAbstraction(g);
            int num_action = getNumAction(g);
            float p[ACTION_SIZE];
            assert(bp.sum_pi[infoset] > 1e-5);
            for (int i = 0; i < num_action; ++i)
                p[i] = bp.sum_sigma[infoset][i] / bp.sum_pi[infoset];
            int action_id = Random::choice(num_action, p);
            Action action = getAction(g, action_id);
            g.doAction(action);
        }
    }
}