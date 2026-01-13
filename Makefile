CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Iinclude
SRC=src/main.c src/camera.c src/scene.c src/renderer.c

all:
	$(CC) $(CFLAGS) $(SRC) -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lopengl32 -lglu32 -lm -o monkey_zoo.exe

linux:
	$(CC) $(CFLAGS) $(SRC) -lSDL2 -lSDL2_image -lGL -lGLU -lm -o monkey_zoo