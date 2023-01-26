CC := clang++
CFLAGS := -Wall -Wextra -Werror -pedantic -g
OBJS := main.o error.o lex.o

# TODO: debug and release targets
all: golf

golf: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

-include $(OBJS:.o=.d)

%.o: %.cpp
	$(CC) -o $@ -c $< $(CFLAGS) -MMD -MF $*.d

clean:
	rm -f *.o *.d golf

# NOTE: some rules for myself that build with zig for ubsan n stuff??
