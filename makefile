NAME = COR
FILES = sockets.c select.c
CFLAGS = -Wall -O3

all:
	gcc -o $(NAME) main.c $(FILES) $(CFLAGS)

teste:
	gcc -o teste teste.c $(FILES) $(CFLAGS)

clean:
	rm -rf teste $(NAME)