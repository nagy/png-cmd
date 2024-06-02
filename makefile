OBJS	= main.o png_chunk.o utilities.o
SOURCE	= src/main.c src/png_chunk.c src/utilities.c
HEADER	= src/png-cmd.h
OUT	= png-cmd
# CC	 = gcc
CFLAGS	 = -std=c2x -g -c -Ofast
LFLAGS	 = 

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

main.o: src/main.c
	$(CC) $(CFLAGS) src/main.c

png_chunk.o: src/png_chunk.c
	$(CC) $(CFLAGS) src/png_chunk.c

utilities.o: src/utilities.c
	$(CC) $(CFLAGS) src/utilities.c

clean:
	rm -f $(OBJS) $(OUT)


.PHONY: clean
