CC = cc
CFLAGS = -g -I.

LEX = flex
YACC = byacc

all: cl

OBJS = node.o atom.o hashtable.o graph.o

y.tab.c y.tab.h: grammar.y
	$(YACC) -v -d grammar.y

lex.yy.c: lex.l
	$(LEX) lex.l

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
