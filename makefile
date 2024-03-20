# To compile the program: make
#----------------------------------------------------------------------

NAME = COR
FILES = sockets.c select.c functionalities.c forwarding.c
CFLAGS = -Wall -O3 -g

all:
	gcc -o $(NAME) main.c $(FILES) $(CFLAGS)

clean:
	rm -rf $(NAME)