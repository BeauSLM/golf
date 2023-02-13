CXX      := clang++
CXXFLAGS := -Wall -Wextra
OBJS     := main.o error.o lex.o parse.o

release: CXXFLAGS += -O3
release: golf

debug: CXXFLAGS += -g
debug: golf

golf: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

-include $(OBJS:.o=.d)

%.o: %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS) -MMD -MF $*.d

clean:
	rm -f *.o *.d golf

# NOTE: some rules for myself that build with zig for ubsan n stuff??
