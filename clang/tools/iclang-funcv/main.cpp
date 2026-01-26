#include "iclang/FuncV/ELF/FuncV.hpp"

int main(const int argc, char **argv) {
  if (argc < 5) {
    llvm::errs() << "Usage: " << argv[0]
                 << "<old-obj-path> <new-obj-path> <output-path> <funcx-path> "
                    "[dump-output(0/1)]\n";
    return 1;
  }

  const std::string oldObjPath = argv[1];
  const std::string newObjPath = argv[2];
  const std::string outputPath = argv[3];
  const std::string funcXPath = argv[4];

  bool dumpOutput = false;
  if (argc > 5) {
    const int flag = atoi(argv[5]);
    dumpOutput = (flag != 0);
  }

  // Read funcx set
  auto lines = iclang::FileSystem::readLines(funcXPath);
  const std::unordered_set<std::string> funcXSet(lines.begin(), lines.end());

  iclang::funcv::elf::FuncV::run(oldObjPath, newObjPath, outputPath,
                                      funcXSet, dumpOutput);

  return 0;
}