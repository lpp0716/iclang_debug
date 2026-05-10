#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <vector>

#include "llvm/Support/JSON.h"

#include "illvm/FuncV/ELF/FuncV.h"
#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/FileSystem.h"

enum class TaskType {
  ForwardingType,
  ExecutingType
};

class Task {
protected:
  TaskType type;
  std::string name;
  std::string description;
  const Task *father = nullptr;

  Task(const TaskType _type, const std::string &_name,
       const std::string &_description)
      : type(_type), name(_name), description(_description) {}

  virtual std::string usageImpl() const = 0;

public:
  TaskType getType() const { return type; }

  std::string getName() const { return name; }

  std::string getDescription() const { return description; }

  std::string usage() const {
    std::vector<std::string> names;
    names.push_back(name);
    for (auto curTask = father; curTask != nullptr; curTask = curTask->father) {
      names.push_back(curTask->name);
    }
    std::ostringstream oss;
    oss << "Usage:";
    for (int i = names.size() - 1; i >= 0; i--) {
      oss << " " << names[i];
    }
    oss << ":" << std::endl;
    oss << usageImpl();
    return oss.str();
  }

  virtual ~Task() = default;
};

class ForwardingTask : public Task {
protected:
  std::map<std::string, const Task*> children;

  explicit ForwardingTask(const std::string &_name,
                          const std::string &_description,
                          ForwardingTask *_father)
      : Task(TaskType::ForwardingType, _name, _description) {
    if (_father != nullptr) {
      father = _father;
      _father->addChild(this);
    }
  }

  std::string usageImpl() const override {
    std::ostringstream oss;
    for (const auto &[taskName, childTask] : children) {
      oss << "* " << taskName << ": " << childTask->getDescription()
          << std::endl;
    }
    return oss.str();
  }

public:
  void addChild(const Task *childTask) {
    children[childTask->getName()] = childTask;
  }

  const Task* trans(std::queue<std::string> &argQ) const {
    if (argQ.empty()) {
      std::cerr << "Error: Missing task name" << std::endl;
      std::cerr << usage();
      exit(1);
    }
    if (argQ.front() == "help") {
      std::cout << usage();
      exit(0);
    }
    const auto taskName = argQ.front();
    argQ.pop();
    const auto it = children.find(taskName);
    if (it == children.end()) {
      std::cerr << "Error: Invalid task: " << taskName << std::endl;
      std::cerr << usage();
      exit(1);
    }
    return it->second;
  }
};

class ExecutingTask : public Task {
protected:
  std::vector<std::string> argNames;

  explicit ExecutingTask(const std::string &_name,
                         const std::string &_description,
                         ForwardingTask *_father)
      : Task(TaskType::ExecutingType, _name, _description) {
    assert(_father != nullptr);
    father = _father;
    _father->addChild(this);
  }

  std::string usageImpl() const override {
    std::ostringstream oss;
    for (const auto &argName : argNames) {
      oss << "<" << argName << "> ";
    }
    oss << std::endl;
    oss << description << std::endl;
    return oss.str();
  }

  virtual int runImpl(const std::vector<std::string> &argValues) const = 0;

public:
  int run(std::queue<std::string> &argQ) const {
    if (!argQ.empty() && argQ.front() == "help") {
      std::cout << usage();
      return 0;
    }
    std::vector<std::string> argValues;
    argValues.reserve(argNames.size());
    for (const auto &argName : argNames) {
      if (argQ.empty()) {
        std::cerr << "Error: Missing argument " << argName << std::endl;
        std::cerr << usage();
        return 1;
      }
      argValues.emplace_back(argQ.front());
      argQ.pop();
    }
    return runImpl(argValues);
  }
};

class ILLVMTestTask : public ForwardingTask {
private:
  ILLVMTestTask() : ForwardingTask("illvm-test", "", nullptr) {}

public:
  static ILLVMTestTask *getInstance() {
    static ILLVMTestTask instance;
    return &instance;
  }
};

class HelloTask : public ExecutingTask {
private:
  int runImpl(const std::vector<std::string> &argValues) const override {
    const std::string helloContent = argValues[0];
    std::cout << "Hello " << helloContent << std::endl;
    return 0;
  }

  explicit HelloTask(ILLVMTestTask *iLLVMTestTask)
      : ExecutingTask("hello", "Print Hello + helloContent", iLLVMTestTask) {
    argNames.emplace_back("helloContent");
  }

public:
  __attribute__((constructor)) static HelloTask *getInstance() {
    static HelloTask instance(ILLVMTestTask::getInstance());
    return &instance;
  }
};

class JsonCheckTask : public ExecutingTask {
private:
  int runImpl(const std::vector<std::string> &argValues) const override {
    const std::string inputJsonFile = argValues[0];
    const std::string key = argValues[1];
    const std::string value = argValues[2];
    const std::string valueType = argValues[3];

    const auto jsonData = illvm::FileSystem::readAll(inputJsonFile);
    auto valueOrErr = llvm::json::parse(jsonData);
    ILLVM_FATAL_ON(valueOrErr.takeError(),
                   "Can not parse meta data: " + inputJsonFile);
    auto *rootPtr = valueOrErr->getAsObject();
    ILLVM_FCHECK(rootPtr != nullptr,
                 "Can not load object from meta data: " + inputJsonFile);

    auto root = std::move(*rootPtr);

    auto it = root.find(key);
    if (it == root.end()) {
      std::cerr << "Key does not exist: " << key << std::endl;
      return 1;
    }

    std::string originValue;
    if (valueType == "int") {
      const auto valueOpt = it->second.getAsInteger();
      if (!valueOpt.has_value()) {
        std::cerr << "Cannot get int value of key: " << key << std::endl;
        return 1;
      }
      originValue = std::to_string(valueOpt.value());
    } else if (valueType == "string") {
      const auto valueOpt = it->second.getAsString();
      if (!valueOpt.has_value()) {
        std::cerr << "Cannot get string value of key: " << key << std::endl;
        return 1;
      }
      originValue = valueOpt.value();
    } else {
      std::cerr << "Unknown type: " << valueType << std::endl;
      return 1;
    }

    if (originValue != value) {
      std::cerr << "Value does not match: " << originValue << " != " << value
                << std::endl;
      return 1;
    }

    return 0;
  }

  explicit JsonCheckTask(ILLVMTestTask *iLLVMTestTask)
      : ExecutingTask("jsonCheck",
                      "Check if the inputJsonFile has <key, value>, type "
                      "support [int, string]",
                      iLLVMTestTask) {
    argNames.emplace_back("inputJsonFile");
    argNames.emplace_back("key");
    argNames.emplace_back("value");
    argNames.emplace_back("type");
  }

public:
  __attribute__((constructor)) static JsonCheckTask *getInstance() {
    static JsonCheckTask instance(ILLVMTestTask::getInstance());
    return &instance;
  }
};

class FuncVTask : public ForwardingTask {
private:
  explicit FuncVTask(ILLVMTestTask *iLLVMTestTask)
      : ForwardingTask("funcv", "FuncV", iLLVMTestTask) {}

public:
  static FuncVTask *getInstance() {
    static FuncVTask instance(ILLVMTestTask::getInstance());
    return &instance;
  }
};

class FuncVELFTask : public ForwardingTask {
private:
  explicit FuncVELFTask(FuncVTask *funcVTask)
      : ForwardingTask("elf", "FuncV ELF", funcVTask) {}

public:
  static FuncVELFTask *getInstance() {
    static FuncVELFTask instance(FuncVTask::getInstance());
    return &instance;
  }
};

class FuncVELFDumpTask : public ExecutingTask {
private:
  int runImpl(const std::vector<std::string> &argValues) const override {
    const std::string elfFilePath = argValues[0];
    auto objFileOrErr = illvm::funcv::elf::FuncV::loadObjFile(elfFilePath);
    ILLVM_FATAL_ON(objFileOrErr.takeError(), "");
    auto &objFile = *objFileOrErr;

    objFile.layout();
    std::cout << objFile.toString() << std::endl;
    return 0;
  }

  explicit FuncVELFDumpTask(FuncVELFTask *funcVELFTask)
      : ExecutingTask("dump", "Dump ELF file", funcVELFTask) {
    argNames.emplace_back("elfFilePath");
  }

public:
  __attribute__((constructor)) static FuncVELFDumpTask *getInstance() {
    static FuncVELFDumpTask instance(FuncVELFTask::getInstance());
    return &instance;
  }
};

class FuncVELFReuseTask : public ExecutingTask {
private:
  int runImpl(const std::vector<std::string> &argValues) const override {
    const std::string oldObjPath = argValues[0];
    const std::string newObjPath = argValues[1];
    const std::string outputPath = argValues[2];
    const std::string funcXPath = argValues[3];

    // Read funcx set
    auto lines = illvm::FileSystem::readLines(funcXPath);
    const std::unordered_set<std::string> funcXSet(lines.begin(), lines.end());

    ILLVM_FATAL_ON(illvm::funcv::elf::FuncV::run(oldObjPath, newObjPath,
                                                 outputPath, funcXSet),
                   "");

    return 0;
  }

  explicit FuncVELFReuseTask(FuncVELFTask *funcVELFTask)
      : ExecutingTask("reuse", "Reuse ELF file", funcVELFTask) {
    argNames.emplace_back("oldObjPath");
    argNames.emplace_back("newObjPath");
    argNames.emplace_back("outputPath");
    argNames.emplace_back("funcXPath");
  }

public:
  __attribute__((constructor)) static FuncVELFReuseTask *getInstance() {
    static FuncVELFReuseTask instance(FuncVELFTask::getInstance());
    return &instance;
  }
};

int main(const int argc, char **argv) {
  std::queue<std::string> argQ;
  for (int i = 1; i < argc; i++) {
    argQ.push(argv[i]);
  }

  const Task *task = ILLVMTestTask::getInstance();
  while (task->getType() == TaskType::ForwardingType) {
    const auto *forwardingTask = static_cast<const ForwardingTask *>(task);
    task = forwardingTask->trans(argQ);
  }
  const auto *executingTask = static_cast<const ExecutingTask *>(task);
  return executingTask->run(argQ);
}