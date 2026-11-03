.POSIX:

include config.mk

SRC = st.c x.c
OBJ = $(SRC:.c=.o)

all: st

.c.o:
	$(CC) $(STCFLAGS) -c $<

st.o: config.h st.h win.h
x.o: arg.h config.h st.h win.h

st: $(OBJ)
	$(CC) -o $@ $(OBJ) $(STLDFLAGS)

x.ll: x.c arg.h config.h st.h win.h
	$(CLANG) -S -emit-llvm -g -O0 -o $@ $< $(STLDFLAGS)

callgraph.o: tools/callgraph.cpp
	$(CXX) -c -I /usr/include/llvm-19 -I /usr/include/llvm-c-19 $^ -o $@

callgraph: callgraph.o
	$(CXX) $^ -L /usr/lib/llvm-19/lib `llvm-config-19 --libs` -o $@

unreachable: callgraph x.ll
	./callgraph x.ll
