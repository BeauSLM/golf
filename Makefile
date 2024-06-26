CXX      := clang++
CXXFLAGS := -Wall -Wextra -std=c++17
OBJS     := main.o error.o lex.o parse.o ast.o symboltable.o semantics.o codegen.o operator_rules.o

release: CXXFLAGS += -O3 -DMILESTONE=4
release: golf

ms4: CXXFLAGS += -DMILESTONE=4 -g
ms4: golf

ms3: CXXFLAGS += -DMILESTONE=3 -g
ms3: golf

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
