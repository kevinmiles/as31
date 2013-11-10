SOURCES = as31.c run.c lexer.c parser.c symbol.c emitter.c
OBJECTS = $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SOURCES))))

all: $(SOURCES)
	gcc -O3 -Wall -o as31 $(SOURCES)

parser.c:
	bison -d -o parser.c parser.y

clean:
	rm as31 parser.h parser.c
