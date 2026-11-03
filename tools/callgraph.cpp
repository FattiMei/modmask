#include <set>
#include <stack>
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

int main(int argc, char* argv[]) {
	if (argc != 2) {
		errs() << "Usage: callgraph input.ll\n";
		return 1;
	}

	LLVMContext Context;
	SMDiagnostic Err;

	// at this stage, there should be no errors, but we should examine Err
	std::unique_ptr<Module> mod = parseIRFile(argv[1], Err, Context);

	// I need pointers to Function to be stored in an ordered container like std::map
	// but iterating on a module only gives you references to Function...
	// turns out we can take pointer to references!
	std::set<Function*> reached;

	Function* main = mod->getFunction("main");
	if (main == NULL) {
		outs() << "Could not find main (entry point)... quitting\n";
		return 1;
	}

	// now we perform the graph reachability algorithm
	std::stack<Function*> queue;
	queue.push(main);
	reached.insert(main);

	// when functions have their address taken, we must make the conservative assumption
	// that they will be called.
	//
	// we add those functions as starting points in the graph reachability algorithm
	for (Function &F : *mod) {
		if (F.hasAddressTaken()) {
			queue.push(&F);
			reached.insert(&F);
		}
	}

	while (not queue.empty()) {
		Function* f = queue.top();
		queue.pop();

		for (BasicBlock &BB : *f) {
			for (Instruction &I : BB) {
				if (auto *call = dyn_cast<CallInst>(&I)) {
					Function* g = call->getCalledFunction();

					if (g == NULL or g->empty()) {
						continue;
					}

					if (reached.count(g) == 0) {
						reached.insert(g);
						queue.push(g);
					}
				}
			}
		}
	}

	for (Function &F : *mod) {
		const bool is_not_empty = not F.empty();
		const bool unreachable = reached.count(&F) == 0;

		if (is_not_empty and unreachable) {
			StringRef function_name = F.getName();
			DISubprogram* prg = F.getSubprogram();

			if (prg != NULL) {
				outs() << prg->getFilename()     // name of file
				       << ":" << prg->getLine()  // line number
				       << ":0: ";                // column number
			}

			outs() << "`" << function_name << "` is unreachable from `main`\n";
		}
	}

	return 0;
}

