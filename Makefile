.POSIX:


include config.mk


# this should be obtained with the program `llvm-config --version`
# I should use awk to recover the version, but I'll wait for a CMake build
# LLVM_VERSION = `llvm-config --version | cut -d . -f 1`
LLVM_VERSION = 20
CLANG = clang-$(LLVM_VERSION)
LLVM = llvm-$(LLVM_VERSION)
LLVM_C = llvm-c-$(LLVM_VERSION)
LLVM_CONFIG = llvm-config-$(LLVM_VERSION)


SRC = st.c x.c
IR = $(SRC:.c=.ll)


all: st


st.ll: st.c arg.h config.h st.h win.h
	$(CLANG) -g -S -emit-llvm $(INCS) $(STCPPFLAGS) -o $@ $<


x.ll: x.c config.h st.h win.h
	$(CLANG) -g -S -emit-llvm $(INCS) $(STCPPFLAGS) -o $@ $<


st: st.ll x.ll
	$(CLANG) -o $@ $^ $(STLDFLAGS)


callgraph: tools/callgraph.cpp
	$(CXX) -I /usr/include/$(LLVM) -I /usr/include/$(LLVM_C) $^ -L /usr/lib/$(LLVM)/lib `$(LLVM_CONFIG) --libs` -o $@


unreachable: callgraph x.ll st.ll
	./callgraph x.ll st.ll


clean:
	rm -f $(IR) callgraph
