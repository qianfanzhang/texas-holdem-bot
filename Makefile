CXX = g++

PROGRAMS = dealer example_player bp_player

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)

clean_log:
	rm -f ./logs/*

dealer: game.cpp game.h evalHandTables rng.cpp rng.h dealer.cpp net.cpp net.h
	$(CXX) -o $@ game.cpp rng.cpp dealer.cpp net.cpp -Wno-literal-suffix

example_player: game.cpp game.h evalHandTables rng.cpp rng.h example_player.cpp net.cpp net.h
	$(CXX) -o $@ game.cpp rng.cpp example_player.cpp net.cpp -Wno-literal-suffix

bp_player: agent/adapter.cpp agent/adapter.h agent/blueprint.h agent/game.cpp agent/game.h agent/abstraction.cpp agent/abstraction.h agent/cards.cpp agent/cards.h agent/random.h game.cpp game.h evalHandTables rng.cpp rng.h bp_player.cpp net.cpp net.h
	$(CXX) -o $@ agent/adapter.cpp agent/game.cpp agent/abstraction.cpp agent/cards.cpp game.cpp rng.cpp bp_player.cpp net.cpp -Wno-literal-suffix
