all:
	gcc -Isrc/include -Lsrc/lib -o balls src/balls.c -lmingw32 -lSDL2main -lSDL2
