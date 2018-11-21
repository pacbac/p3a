#NAME: Clayton Chu and George Zhang
#ID: 104906833
#EMAIL: claytonchu99@gmail.com

CC=gcc
FLAGS=-g -Wall -Wextra -lm

default:
	$(CC) lab3a.c $(FLAGS) -o lab3a

dist:
	tar -czvf lab3a-104906833.tar.gz lab3a.c README Makefile ext2_fs.h

clean:
	rm -f lab3a *.tar.gz
