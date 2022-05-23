CC = gcc
SRC = simple-shell.c
OBJ = simple-shell.out

shell: $(SRC)
	$(CC) -o $(OBJ) $(SRC)