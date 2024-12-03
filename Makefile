

INCLUDES := include/

STD := c++23

TESTS := empty simple-patterns match

CXX = g++

CXXFLAGS = -O2 -g

CPPFLAGS = 

ALL_CPPFLAGS := $(INCLUDES:%=-I%) $(CPPFLAGS)

ALL_CXXFLAGS := -std=$(STD) $(CXXFLAGS)

LDFLAGS ?=

all: $(TESTS:%=out/%)


.PHONY: all clean test $(TESTS:%=test-%)

clean:
	rm -rf out

test: $(TESTS:%=out/%.stamp)

$(TESTS:%=test-%): test-%: out/%.stamp

out/%.stamp: out/%
	@echo Running Test $(<:out/%=%)
	@+$<
	@touch $@

out/:
	mkdir -p out

$(TESTS:%=out/%): out/%: out/%.o
	$(CXX) $(ALL_CXXFLAGS) -o $@ $< $(LDFLAGS)

out/%.o: tests/%.cxx out/%.o.d
	$(CXX) $(ALL_CPPFLAGS) $(ALL_CXXFLAGS) -MMD -MF $@.d -o $@ -c $<

out/%.o.d: tests/%.cxx out/
	$(CXX) $(ALL_CPPFLAGS) $(ALL_CXXFLAGS) -MM -MF $@ -MT $(@:out/%.d=out/%) $<

include $(TESTS:%=out/%.o.d)