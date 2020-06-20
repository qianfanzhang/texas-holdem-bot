CXX = g++

# PROGRAMS = abstraction
PROGRAMS = blueprint interact

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)

abstraction: abstraction.cpp abstraction.h game.h game.cpp cards.h cards.cpp random.h evalHandTables
	$(CXX) -o $@ abstraction.cpp game.cpp cards.cpp -Wno-literal-suffix -O2 -pthread

abstraction_gen: abstraction_gen.cpp abstraction.cpp abstraction.h game.h game.cpp cards.h cards.cpp random.h evalHandTables
	$(CXX) -o $@ abstraction_gen.cpp abstraction.cpp game.cpp cards.cpp -Wno-literal-suffix -O2

blueprint: blueprint.cpp blueprint.h abstraction.cpp abstraction.h game.h game.cpp cards.h cards.cpp random.h evalHandTables
	$(CXX) -o $@ blueprint.cpp abstraction.cpp game.cpp cards.cpp -Wno-literal-suffix -Wall -Wextra -Wshadow -pedantic -O2

interact: interact.cpp blueprint.h abstraction.cpp abstraction.h game.h game.cpp cards.h cards.cpp random.h evalHandTables
	$(CXX) -o $@ interact.cpp abstraction.cpp game.cpp cards.cpp -Wno-literal-suffix -Wall -Wextra -Wshadow -pedantic -O2
