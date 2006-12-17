#CC = tcc -Wall
#CFLAGS = -I. -D_TCC_
#YACC = yacc  -d -v

CC = gcc
CFLAGS = -I. -g -O -Wall
YACC = bison -b y

#CC = lcc
#CFLAGS = -I.

#LEX = lex
#YACC = yacc -d -v

all: cl

OBJS = node.o atom.o hashtable.o graph.o arena.o abbreviations.o \
	spine_stack.o bracket_abstraction.o

y.tab.c y.tab.h: grammar.y
	$(YACC) -d grammar.y

lex.yy.c: lex.l
	$(LEX) lex.l

lex.yy.o: lex.yy.c y.tab.h atom.h hashtable.h node.h

y.tab.o: y.tab.c y.tab.h node.h atom.h hashtable.h graph.h \
	abbreviations.h bracket_abstraction.h

node.o: node.c node.h arena.h
atom.o: atom.c atom.h hashtable.h
hashtable.o: hashtable.c hashtable.h
graph.o: graph.c graph.h node.h spine_stack.h
arena.o: arena.c arena.h
abbreviations.o: abbreviations.c abbreviations.h node.h hashtable.h
spine_stack.o: spine_stack.c spine_stack.h
bracket_abstraction.o: bracket_abstraction.c bracket_abstraction.h node.h

cl: y.tab.o lex.yy.o $(OBJS)
	$(CC) -g -o cl y.tab.o lex.yy.o $(OBJS)

clean:
	-rm -rf cl
	-rm y.tab.c y.tab.h lex.yy.c y.output
	-rm -rf y.tab.o lex.yy.o $(OBJS)
	-rm -rf core *.a *.o
	-rm -f tests.output/*
