.PHONY: all release clean run_steiner coverage
.DEFAULT_GOAL:=all
.EXTRA_PREREQS=Makefile

CXXFLAGS_STD:=-std=c++14
CXXFLAGS_OPT:=-Os -g --coverage
CXXFLAGS_WARN:=-Wall -Werror -Wextra -Wno-error=extra -Wunused -Wno-error=unused -Wunused-value -Wno-error=unused-value
CPPFLAGS_DEPS:=-MMD -MP

DIR_BUILD:=build

DIR_OBJS:=$(DIR_BUILD)/objs
DIR_SRC:=src
DIR_SRC_steiner:=src/common src/math src/filetypes

EXEC_steiner:=$(DIR_BUILD)/steiner.out
EXECS+=$(EXEC_steiner)
# Library support
LDLIBS_steiner:=$(addprefix -l,SDL2 SDL2_image SDL2_mixer gcov stdc++)
# Standard
CXXFLAGS_steiner:=$(CXXFLAGS_STD) $(CXXFLAGS_OPT)
# Make sure we handle dependencies.
# MMD gets .d files, MP adds header handling
# To move the file to a uniform place, add -MF
CPPFLAGS_steiner:=$(CPPFLAGS_DEPS) $(addprefix -I,$(DIR_SRC_steiner))

print=$(info $(1) $($(1)))

SRC_steiner:=$(foreach d,$(DIR_SRC) $(DIR_SRC_steiner),$(wildcard $(d)/*.cpp))
DIR_OBJS_steiner:=$(DIR_OBJS)/steiner
OBJS_steiner:=$(patsubst $(DIR_SRC)/%,$(DIR_OBJS_steiner)/%.o,$(SRC_steiner))
DIR_OBJS:=$(dir $(OBJS_steiner))

MKDIR:=mkdir -p

all: $(EXECS)
	echo "Build $@ completed"

$(EXEC_steiner) : $(OBJS_steiner)
$(EXEC_steiner) : LDLIBS:=$(LDLIBS_steiner)
$(EXEC_steiner) $(OBJS_steiner) : CPPFLAGS:=$(CPPFLAGS_steiner)
$(EXEC_steiner) $(OBJS_steiner) : CXXFLAGS:=$(CXXFLAGS_steiner)

$(OBJS_steiner) : $(DIR_OBJS_steiner)/%.cpp.o : $(DIR_SRC)/%.cpp
	echo Making $@
	$(MKDIR) $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $^ 
	echo Made   $@

$(EXECS) :
	echo Making $@
	$(MKDIR) $(@D)
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@
	echo Made $@

run_steiner : $(EXEC_steiner)
	./$(EXEC_steiner)
	
clean :
	rm -r $(DIR_BUILD)

coverage : build/coverage/index.htm

build/coverage/index.htm : build/coverage.info
	genhtml build/coverage.info -o build/coverage

build/coverage.info : $(OBJS:%.o=%.gcda)
	lcov -d build -b . -c -o build/coverage.info
	
