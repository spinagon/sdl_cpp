CC = g++
SDL_DIR = c:/prog/SDL3
CFLAGS = -std=c++17 -Werror -O3
CFLAGS += -I$(SDL_DIR)/i686-w64-mingw32/include/
LDFLAGS = -lm
LDFLAGS += -L$(SDL_DIR)/i686-w64-mingw32/lib -lmingw32 -lSDL3_test 
LDFLAGS += -lSDL3 -lSDL3_image

BUILD_DIR = .

all: main.exe graph.exe sort.exe circle.exe clustering.exe inpaint.exe

%.exe: %.o
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^ $(LDFLAGS)

%.o: %.cc
	$(CC) -c $(CFLAGS) -o $@ $^

run_%: %.exe
	$(BUILD_DIR)/$(patsubst run_%,%.exe,$@)

clean:
	rm -rf *.o $(BUILD_DIR)/*

rebuild: clean all

.NOTINTERMEDIATE: