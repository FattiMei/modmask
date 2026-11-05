.POSIX:


include config.mk


# this should be obtained with the program `llvm-config --version`
# I should use awk to recover the version, but I'll wait for a CMake build
LLVM_VERSION = 20
CLANG = clang-$(LLVM_VERSION)
LLC = llc-$(LLVM_VERSION)


SRC = st.c x.c
IR = $(SRC:.c=.ll)


all: st


st.ll: x.c arg.h config.h st.h win.h
	$(CLANG) -g -S -emit-llvm $(INCS) $(STCPPFLAGS) -o $@ $<


x.ll: x.c config.h st.h win.h
	$(CLANG) -g -S -emit-llvm $(INCS) $(STCPPFLAGS) -o $@ $<


st.o: st.ll
	$(LLC) $<


x.o: x.ll
	$(LLC) $<


st: $(IR)
	$(CLANG) -o $@ $^ $(STLDFLAGS)


callgraph.o: tools/callgraph.cpp
	$(CXX) -c -I /usr/include/llvm-$(LLVM_VERSION) -I /usr/include/llvm-c-$(LLVM_VERSION) $^ -o $@


callgraph: tools/callgraph.cpp
	$(CXX) -I /usr/include/llvm-$(LLVM_VERSION) -I /usr/include/llvm-c-$(LLVM_VERSION) $^ -L /usr/lib/llvm-$(LLVM_VERSION)/lib `llvm-config-$(LLVM_VERSION) --libs` -o $@


unreachable: callgraph x.ll st.ll
	./callgraph x.ll st.ll


clean:
	rm -f $(IR) callgraph
