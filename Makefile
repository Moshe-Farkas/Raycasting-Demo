CFLAGS := -Wall
LIBS := -lSDL2 -lm
CC := gcc

program: raycasting.c
	$(CC) $(CFLAGS) raycasting.c $(LIBS) -o program

run: program
	./program
