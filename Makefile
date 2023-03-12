# TODO: don't include debug info on release build
CXX      := clang++
CXXFLAGS := -Wall -Wextra -std=c++17
OBJS     := main.o error.o lex.o parse.o ast.o symboltable.o

# TODO: RELEASE BUILD FOR SUBMISSION
# XXX: I'M NOT KIDDING DO NOT FORGET THIS
ms3: CXXFLAGS += -DMILESTONE=3 -g
ms3: golf

release: CXXFLAGS += -O3 -DMILESTONE=3
release: golf

ms2: CXXFLAGS += -DMILESTONE=2 -g
ms2: golf

ms1: CXXFLAGS += -DMILESTONE=1 -g
ms1: golf

golf: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

-include $(OBJS:.o=.d)

%.o: %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS) -MMD -MF $*.d

clean:
	rm -f *.o *.d golf

# NOTE: some rules for myself that build with zig for ubsan n stuff??
