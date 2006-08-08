CC = cc
CFLAGS = -g -I.

all: cl

OBJS = node.o atom.o hashtable.o graph.o

y.tab.c y.tab.h: cl-grammar-2.y
	yacc -v -d cl-grammar-2.y

lex.yy.c: lex.l
	lex lex.l

lex.yy.o: lex.yy.c y.tab.h atom.h hashtable.h

y.tab.o: y.tab.c y.tab.h node.h atom.h hashtable.h graph.h

node.o: node.c node.h
atom.o: atom.c atom.h hashtable.h
hashtable.o: hashtable.c hashtable.h
graph.o: graph.c graph.h node.h

cl: y.tab.o lex.yy.o $(OBJS)
	$(CC) -g -o cl y.tab.o lex.yy.o $(OBJS)

clean:
	-rm -rf cl
	-rm y.tab.c y.tab.h lex.yy.c y.output
	-rm -rf y.tab.o lex.yy.o $(OBJS)
	-rm -rf core *.a *.o
	-rm -rf tests.output/*
