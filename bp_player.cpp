#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <getopt.h>
#include "game.h"
#include "rng.h"
#include "net.h"

#include "agent/adapter.h"

const int MAX_LEN = 1e5;
char out_str[MAX_LEN];
double acc_utility;
int num_rounds;

Action act(Game *game, MatchState *state, rng_state_t *rng) {
    printMatchState(game, state, MAX_LEN, out_str);
    printf("%s\n", out_str);
    // printf("\tround: %d\n", state->state.round);
    // printf("\tspent: %d %d\n", state->state.spent[0], state->state.spent[1]);
    // printf("\tminRaise: %d\n", state->state.minNoLimitRaiseTo);
    // printf("\tplayer: %d\n", state->viewingPlayer);
    // printf("\tboardCards:");
    // for (int i = 0; i < 5; ++i)
    //     printf("%u ", state->state.boardCards[i]);
    // printf("\n\tholeCards[%d]:", state->viewingPlayer);
    // for (int i = 0; i < 2; ++i)
    //     printf("%u ", state->state.holeCards[state->viewingPlayer][i]);
    // printf("\n");
    // for (int i = 0; i <= state->state.round; ++i) {
    //     printf("\tRound %d: ", i);
    //     for (int j = 0; j < state->state.numActions[i]; ++j) {
    //         printf("(%d:%d,%d) ", state->state.actingPlayer[i][j], state->state.action[i][j].type, state->state.action[i][j].size);
    //     }
    //     printf("\n");
    // }

    bool always_fold = false;
    // if (acc_utility - 76 * (1000 - num_rounds) > 1000)
    //     always_fold = true;

    bool raised[2];
    raised[0] = raised[1] = false;
    for (int i = 0; i <= state->state.round; ++i) {
        for (int j = 0; j < state->state.numActions[i]; ++j) {
            ActionType t = state->state.action[i][j].type;
            if (t == a_raise)
                raised[state->state.actingPlayer[i][j]] = true;
        }
    }
    printf("\t\tRaised: %d %d\n", raised[0], raised[1]);

    State s = state->state;
    int p = state->viewingPlayer;
    std::pair<int, int> a = getAction(s.round, p, s.spent, s.minNoLimitRaiseTo, s.holeCards[p], s.boardCards, "agent/blueprint_checkpoint.txt", "agent/", raised, always_fold);

    Action action;
    action.type = ActionType(a.first);
    action.size = a.second;
    printf("\tDo action (%d,%d)\n", action.type, action.size);

    return action;
}


/* Anything related with socket is handled below. */
/* If you are not interested with protocol details, you can safely skip these. */

int step(int len, char line[], Game *game, MatchState *state, rng_state_t *rng) {
    /* add a colon (guaranteed to fit because we read a new-line in fgets) */
    line[ len ] = ':';
    ++len;

    Action action = act(game, state, rng);

    /* do the action! */
    assert( isValidAction( game, &(state->state), 0, &action ) );
    int r = printAction( game, &action, MAX_LINE_LEN - len - 2, &line[ len ] );
    if( r < 0 ) {
        fprintf( stderr, "ERROR: line too long after printing action\n" );
        exit( EXIT_FAILURE );
    }
    len += r;
    line[ len ] = '\r';
    ++len;
    line[ len ] = '\n';
    ++len;

    return len;
}


int main( int argc, char **argv )
{
    srand(time(NULL));

    int sock, len;
    uint16_t port;
    Game *game;
    MatchState state;
    FILE *file, *toServer, *fromServer;
    struct timeval tv;
    rng_state_t rng;
    char line[ MAX_LINE_LEN ];

    /* we make some assumptions about the actions - check them here */
    assert( NUM_ACTION_TYPES == 3 );

    if( argc < 3 ) {

        fprintf( stderr, "usage: player server port\n" );
        exit( EXIT_FAILURE );
    }

    /* Initialize the player's random number state using time */
    gettimeofday( &tv, NULL );
    init_genrand( &rng, tv.tv_usec );

    /* get the game */
    file = fopen( "./holdem.nolimit.2p.reverse_blinds.game", "r" );
    if( file == NULL ) {

        fprintf( stderr, "ERROR: could not open game './holdem.nolimit.2p.reverse_blind.game'\n");
        exit( EXIT_FAILURE );
    }
    game = readGame( file );
    if( game == NULL ) {

        fprintf( stderr, "ERROR: could not read game './holdem.nolimit.2p.reverse_blind.game'\n");
        exit( EXIT_FAILURE );
    }
    fclose( file );

    /* connect to the dealer */
    if( sscanf( argv[ 2 ], "%"SCNu16, &port ) < 1 ) {

        fprintf( stderr, "ERROR: invalid port %s\n", argv[ 2 ] );
        exit( EXIT_FAILURE );
    }
    sock = connectTo( argv[ 1 ], port );
    if( sock < 0 ) {

        exit( EXIT_FAILURE );
    }
    toServer = fdopen( sock, "w" );
    fromServer = fdopen( sock, "r" );
    if( toServer == NULL || fromServer == NULL ) {

        fprintf( stderr, "ERROR: could not get socket streams\n" );
        exit( EXIT_FAILURE );
    }

    /* send version string to dealer */
    if( fprintf( toServer, "VERSION:%"PRIu32".%"PRIu32".%"PRIu32"\n",
                 VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION ) != 14 ) {

        fprintf( stderr, "ERROR: could not get send version to server\n" );
        exit( EXIT_FAILURE );
    }
    fflush( toServer );

    acc_utility = 0;

    /* play the game! */
    while( fgets( line, MAX_LINE_LEN, fromServer ) ) {

        /* ignore comments */
        if( line[ 0 ] == '#' || line[ 0 ] == ';' ) {
            continue;
        }

        len = readMatchState( line, game, &state );
        if( len < 0 ) {

            fprintf( stderr, "ERROR: could not read state %s", line );
            exit( EXIT_FAILURE );
        }

        if( stateFinished( &state.state ) ) {
            /* ignore the game over message */
            ++num_rounds;
            printMatchState(game, &state, MAX_LEN, out_str);
            printf("%s\n", out_str);
            double u = valueOfState(game, &state.state, state.viewingPlayer);
            acc_utility += u;
            printf("\tUtility:      %.2f\n", u);
            printf("\tAcc. utility: %.2f\n\n", acc_utility);
            continue;
        }

        if( currentPlayer( game, &state.state ) != state.viewingPlayer ) {
            /* we're not acting */

            continue;
        }

        len = step(len, line, game, &state, &rng);

        if( fwrite( line, 1, len, toServer ) != len ) {

            fprintf( stderr, "ERROR: could not get send response to server\n" );
            exit( EXIT_FAILURE );
        }
        fflush( toServer );
    }

    return EXIT_SUCCESS;
}
