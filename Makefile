# TODO: don't include debug info on release build
CXX      := clang++
CXXFLAGS := -Wall -Wextra -std=c++17 -g
OBJS     := main.o error.o lex.o parse.o

release: CXXFLAGS += -O3
release: ms3

ms3: CXXFLAGS += -DMILESTONE=3
ms3: golf

ms2: CXXFLAGS += -DMILESTONE=2
ms2: golf

ms1: CXXFLAGS += -DMILESTONE=1
ms1: golf

golf: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

-include $(OBJS:.o=.d)

%.o: %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS) -MMD -MF $*.d

clean:
	rm -f *.o *.d golf

# NOTE: some rules for myself that build with zig for ubsan n stuff??
