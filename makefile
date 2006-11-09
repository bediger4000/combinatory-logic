CC = lcc
CFLAGS = -g -I. 

LEX = flex
YACC = byacc

all: cl

OBJS = node.o atom.o hashtable.o graph.o arena.o abbreviations.o \
	spine_stack.o

y.tab.c y.tab.h: grammar.y
	$(YACC) -v -d grammar.y

lex.yy.c: lex.l
	$(LEX) lex.l

lex.yy.o: lex.yy.c y.tab.h atom.h hashtable.h node.h

y.tab.o: y.tab.c y.tab.h node.h atom.h hashtable.h graph.h abbreviations.h

node.o: node.c node.h arena.h
atom.o: atom.c atom.h hashtable.h
hashtable.o: hashtable.c hashtable.h
graph.o: graph.c graph.h node.h spine_stack.h
arena.o: arena.c arena.h
abbreviations.o: abbreviations.c abbreviations.h node.h hashtable.h
spine_stack.o: spine_stack.c spine_stack.h

cl: y.tab.o lex.yy.o $(OBJS)
	$(CC) -g -o cl y.tab.o lex.yy.o $(OBJS)

clean:
	-rm -rf cl
	-rm y.tab.c y.tab.h lex.yy.c y.output
	-rm -rf y.tab.o lex.yy.o $(OBJS)
	-rm -rf core *.a *.o
	-rm -f tests.output/*
