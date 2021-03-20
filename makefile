CFLAGS=-Wall -Wextra -Werror -Wshadow -g
LIBS=-lSDL2
INCL=-Iinclude -Isrc/include
INT=src/include/internal.h
AR=libdos.a
O=obj
OBJS=$(O)/dos.o \
	$(O)/conio.o \
	$(O)/graphics.o \
	$(O)/font.o

# let's keep this simple...

all: lib test

lib: $(OBJS)
	ar rcs $(AR) $^

$(O)/dos.o: src/dos.c include/dos.h $(INT)
	cc -c src/dos.c -o $@ $(INCL) $(CFLAGS)
	
$(O)/conio.o: src/conio.c include/conio.h $(INT)
	cc -c src/conio.c -o $@  $(INCL) $(CFLAGS)
	
$(O)/graphics.o: src/graphics.c include/conio.h $(INT)
	cc -c src/graphics.c -o $@  $(INCL) $(CFLAGS)

$(O)/font.o: src/font.c
	cc -c src/font.c -o $@ $(INCL) $(CFLAGS)
	
test: $(OBJS) $(O)/test.o
	cc $^ -o $@ $(LIBS) $(INCL)
	
$(O)/test.o: test.c
	cc -c $< -o $@ $(CFLAGS) $(INCL)
	
.PHONY: clean
clean:
	@rm -rf $(O)/* test $(AR)
