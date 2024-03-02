NAME = COR
FILES = sockets.c select.c functionalities.c
CFLAGS = -Wall -O3 -g

all:
	gcc -o $(NAME) main.c $(FILES) $(CFLAGS)

teste:
	gcc -o teste teste.c $(FILES) $(CFLAGS)

clean:
	rm -rf teste $(NAME)