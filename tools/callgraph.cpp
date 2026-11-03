#include <iostream>
#include "llvm/IR/Module.h"

using namespace llvm;


int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: callgraph input.ll\n";
		return 1;
	}

	const char* input_ir_filename = argv[1];

	std::cout << "Opening file " << input_ir_filename << std::endl;

	return 0;
}
