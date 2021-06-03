RETRO_BC=libretro.bc
SRC=src

CFLAGS = -std=c99
CFLAGS += -W -Wall -Wno-unused-parameter
CFLAGS += -I$(SRC)
CFLAGS += -O3

CXXFLAGS = -std=c++11
CXXFLAGS += -W -Wall -Wno-unused-parameter
CXXFLAGS += -I$(SRC)
CXXFLAGS += -O3

%.c.o: %.c
	emcc -c -o $(@) $(<) $(CFLAGS)

%.cpp.o: %.cpp
	emcc -c -o $(@) $(<) $(CXXFLAGS)

SOURCES = \
  $(SRC)/input.c.o \
  $(SRC)/audio.c.o \
  $(SRC)/environment.c.o \
  $(SRC)/video.c.o \
  $(SRC)/frontend.c.o \
  $(SRC)/main.c.o \
  $(SRC)/options.cpp.o

all:$(RETRO_BC)

$(RETRO_BC): $(SOURCES)
	emcc $(SOURCES) \
		--bind \
		-s ALLOW_MEMORY_GROWTH=1 \
		-r -o $(RETRO_BC)

clean:
	@$(RM) -f $(SRC)/*.o
	@$(RM) -f $(RETRO_BC)

.PHONY: all clean
