TARGET=libdos.a
CFLAGS=-Wall
LIBS=-lSDL2
INCL=-Iinclude -Isrc/include
OBJS=dos.o conio.o graphics.o font.o internal.o

$(TARGET): $(OBJS)
	ar rcs $(TARGET) $^

%.o: src/%.c include/%.h src/include/internal.h
	cc -c $< -o $@ $(INCL) $(CFLAGS)

internal.o: src/internal.c src/include/internal.h
	cc -c $< -o $@ $(INCL) $(CFLAGS)

font.o: src/font.c
	cc -c $^ -o $@ $(CFLAGS)

install: $(TARGET)
	cp -i $(TARGET) /usr/local/lib
	cp -i include/* /usr/local/include

.PHONY: clean
clean:
	@rm -rf *.o $(TARGET)
