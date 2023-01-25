CC := clang++
OBJS := main.o error.o

all: golf

golf: $(OBJS)
	$(CC) -Wall -o $@ $^

-include $(OBJS:.o=.d)

%.o: %.cpp
	$(CC) -o $@ -c $< -Wall -MMD -MF $*.d

clean:
	rm -f *.o *.d golf

# NOTE: some rules for myself that build with zig for ubsan n stuff??
