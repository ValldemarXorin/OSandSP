CC = g++
CFLAGS = -std=c++11 -Wall -Wextra -Wpedantic -g
MACRO = macro11
SOURCES = Ast.cpp CodeGenerator.cpp Compiler.cpp ErrorHandling.cpp lex.yy.c $(MACRO).tab.c SemanticAnalyzer.cpp Utils.cpp

ALL:
	flex $(MACRO).l
	bison -d $(MACRO).y
	$(CC) $(CFLAGS) $(SOURCES) -o $(MACRO)

clean:
	rm -f *.o $(MACRO) lex.yy.c $(MACRO).tab.*