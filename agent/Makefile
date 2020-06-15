CXX = g++

# PROGRAMS = abstraction
PROGRAMS = blueprint

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)

abstraction: abstraction.cpp abstraction.h game.h game.cpp cards.h cards.cpp random.h random.cpp evalHandTables
	$(CXX) -o $@ abstraction.cpp game.cpp cards.cpp random.cpp -Wno-literal-suffix

blueprint: blueprint.cpp blueprint.h abstraction.cpp abstraction.h game.h game.cpp cards.h cards.cpp random.h random.cpp evalHandTables
	$(CXX) -o $@ blueprint.cpp abstraction.cpp game.cpp cards.cpp random.cpp -Wno-literal-suffix -Wall -Wextra -Wshadow -pedantic -O2

# blueprint: blueprint.cpp blueprint.h abstraction.cpp abstraction.h game.h game.cpp cards.h cards.cpp random.h random.cpp evalHandTables
# 	$(CXX) -o $@ blueprint.cpp abstraction.cpp game.cpp cards.cpp random.cpp -Wno-literal-suffix -mcmodel=medium -O2 -fsanitize=address -fsanitize=undefined -fsanitize=float-divide-by-zero