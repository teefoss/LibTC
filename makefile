CFLAGS=-Wall -Wextra -Werror -Wshadow -g -ansi -pedantic
LIBS=-lSDL2
INCL=-Iinclude -Isrc/include
INT=src/include/internal.h
AR=libdos.a
O=obj
OBJS=$(O)/dos.o \
	$(O)/conio.o \
	$(O)/graphics.o \
	$(O)/font.o

all: lib test

lib: $(OBJS)
	ar rcs $(AR) $^

$(O)/%.o: src/%.c include/%.h $(INT)
	cc -c $< -o $@ $(INCL) $(CFLAGS)

$(O)/font.o: src/font.c
	cc -c src/font.c -o $@ $(INCL) $(CFLAGS)
	
test: test.c $(OBJS)
	cc $< -o $@ $(LIBS) -ldos $(INCL)
		
.PHONY: clean
clean:
	@rm -rf $(O)/* test $(AR)
