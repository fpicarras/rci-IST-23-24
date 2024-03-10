NAME = COR
FILES = sockets.c select.c functionalities.c fowarding.c
CFLAGS = -Wall -O3 -g

all:
	gcc -o $(NAME) main.c $(FILES) $(CFLAGS)

teste:
	gcc -o test teste.c $(FILES) $(CFLAGS)

clean:
	rm -rf teste $(NAME)