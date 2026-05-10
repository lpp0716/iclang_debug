#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <sstream>
#include <vector>

#include "llvm/Support/JSON.h"

#include "iclang/Support/Global.h"

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

class IClangStaTask : public ForwardingTask {
private:
  IClangStaTask() : ForwardingTask("iclang-sta", "", nullptr) {}

public:
  static IClangStaTask *getInstance() {
    static IClangStaTask instance;
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

  explicit HelloTask(IClangStaTask *iClangStaTask)
      : ExecutingTask("hello", "Print Hello + helloContent", iClangStaTask) {
    argNames.emplace_back("helloContent");
  }

public:
  __attribute__((constructor)) static HelloTask *getInstance() {
    static HelloTask instance(IClangStaTask::getInstance());
    return &instance;
  }
};

class IncLineStaTask : public ExecutingTask {
private:
  int runImpl(const std::vector<std::string> &argValues) const override {
    const std::string projectPath = argValues[0];

    namespace fs = std::filesystem;

    std::error_code ec;

    std::vector<std::string> compileJsonPaths;

    fs::recursive_directory_iterator it(projectPath, ec), end;
    if (ec) {
      std::cerr << "Error accessing root: " << ec.message() << "\n";
      return 1;
    }

    while (it != end) {
      const fs::directory_entry &entry = *it;

      if (entry.is_regular_file(ec)) {
        const fs::path &p = entry.path();
        if (p.filename() == "compile.json" &&
            p.parent_path().extension() == ".iclang") {
          compileJsonPaths.push_back(p.string());
        }
      }

      it.increment(ec);
      if (ec) {
        std::cerr << "Error during iteration: " << ec.message() << "\n";
        return 1;
      }
    }

    const uint64_t totalFileNum = compileJsonPaths.size();
    uint64_t hashHashFileNum = 0;

    for (const auto &compileJsonPath : compileJsonPaths) {
      const auto jsonData = illvm::FileSystem::readAll(compileJsonPath);
      auto valueOrErr = llvm::json::parse(jsonData);
      ILLVM_FATAL_ON(valueOrErr.takeError(),
                     "Can not parse meta data: " + compileJsonPath);
      auto *rootPtr = valueOrErr->getAsObject();
      ILLVM_FCHECK(rootPtr != nullptr,
                   "Can not load object from meta data: " + compileJsonPath);

      auto root = std::move(*rootPtr);

      if (root.find("hashHashFlag") == root.end()) {
        std::cerr << "Cannot load hashHashFlag from " << compileJsonPath
                  << std::endl;
        return 1;
      }

      const bool hashHashFlag = root["hashHashFlag"].getAsBoolean().value();
      hashHashFileNum += hashHashFlag;
    }

    std::cout << "[totalFileNum] " << totalFileNum << std::endl;
    std::cout << "[hashHashFileNum] " << hashHashFileNum << std::endl;

    return 0;
  }

  explicit IncLineStaTask(IClangStaTask *iClangStaTask)
      : ExecutingTask("incLineSta", "Inc line sta", iClangStaTask) {
    argNames.emplace_back("projectPath");
  }

public:
  __attribute__((constructor)) static IncLineStaTask *getInstance() {
    static IncLineStaTask instance(IClangStaTask::getInstance());
    return &instance;
  }
};

class LineMacroStaTask : public ExecutingTask {
private:
  int runImpl(const std::vector<std::string> &argValues) const override {
    const std::string projectPath = argValues[0];

    namespace fs = std::filesystem;

    std::error_code ec;

    std::vector<std::string> compileJsonPaths;

    fs::recursive_directory_iterator it(projectPath, ec), end;
    if (ec) {
      std::cerr << "Error accessing root: " << ec.message() << "\n";
      return 1;
    }

    while (it != end) {
      const fs::directory_entry &entry = *it;

      if (entry.is_regular_file(ec)) {
        const fs::path &p = entry.path();
        if (p.filename() == "compile.json" &&
            p.parent_path().extension() == ".iclang") {
          compileJsonPaths.push_back(p.string());
        }
      }

      it.increment(ec);
      if (ec) {
        std::cerr << "Error during iteration: " << ec.message() << "\n";
        return 1;
      }
    }

    uint64_t totalFuncNum = 0;
    uint64_t funcWithLineMacroNum = 0;
    const uint64_t totalFileNum = compileJsonPaths.size();
    uint64_t fileWithLineMacroNum = 0;

    for (const auto &compileJsonPath : compileJsonPaths) {
      const auto jsonData = illvm::FileSystem::readAll(compileJsonPath);
      auto valueOrErr = llvm::json::parse(jsonData);
      ILLVM_FATAL_ON(valueOrErr.takeError(),
                     "Can not parse meta data: " + compileJsonPath);
      auto *rootPtr = valueOrErr->getAsObject();
      ILLVM_FCHECK(rootPtr != nullptr,
                   "Can not load object from meta data: " + compileJsonPath);

      auto root = std::move(*rootPtr);

      if (root.find("totalFuncNum") == root.end()) {
        std::cerr << "Cannot load totalFuncNum from " << compileJsonPath
                  << std::endl;
        return 1;
      }
      if (root.find("funcWithLineMacroNum") == root.end()) {
        std::cerr << "Cannot load funcWithLineMacroNum from " << compileJsonPath
                  << std::endl;
        return 1;
      }

      const int _totalFuncNum = root["totalFuncNum"].getAsInteger().value();
      const int _funcWithLineMacroNum =
          root["funcWithLineMacroNum"].getAsInteger().value();

      totalFuncNum += _totalFuncNum;
      funcWithLineMacroNum += _funcWithLineMacroNum;
      if (_funcWithLineMacroNum != 0) {
        std::cout << "Find line macro in " << compileJsonPath << std::endl;
        fileWithLineMacroNum += 1;
      }
    }

    std::cout << "[totalFuncNum] " << totalFuncNum << std::endl;
    std::cout << "[funcWithLineMacroNum] " << funcWithLineMacroNum << std::endl;
    std::cout << "[totalFileNum] " << totalFileNum << std::endl;
    std::cout << "[fileWithLineMacroNum] " << fileWithLineMacroNum << std::endl;

    return 0;
  }

  explicit LineMacroStaTask(IClangStaTask *iClangStaTask)
      : ExecutingTask("lineMacroSta", "Line macro sta", iClangStaTask) {
    argNames.emplace_back("projectPath");
  }

public:
  __attribute__((constructor)) static LineMacroStaTask *getInstance() {
    static LineMacroStaTask instance(IClangStaTask::getInstance());
    return &instance;
  }
};

int main(const int argc, char **argv) {
  std::queue<std::string> argQ;
  for (int i = 1; i < argc; i++) {
    argQ.push(argv[i]);
  }

  const Task *task = IClangStaTask::getInstance();
  while (task->getType() == TaskType::ForwardingType) {
    const auto *forwardingTask = static_cast<const ForwardingTask *>(task);
    task = forwardingTask->trans(argQ);
  }
  const auto *executingTask = static_cast<const ExecutingTask *>(task);
  return executingTask->run(argQ);
}