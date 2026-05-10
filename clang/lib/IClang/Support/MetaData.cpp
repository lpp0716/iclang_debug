#include "iclang/Support/MetaData.h"

#include <iomanip>
#include <sstream>

#include "illvm/Support/Diagnostics.h"

namespace iclang {

std::string MetaData::hackMainBuffer(const std::string &originalBuffer,
                                  const std::vector<std::string> &tir) {
  std::istringstream iss(originalBuffer);
  std::string line;
  std::vector<std::string> lines;

  while (getline(iss, line)) {
    lines.push_back(line);
  }

  std::ostringstream oss;
  for (size_t i = 0; i < tir.size(); i++) {
    for (size_t j = 0; j < tir[i].size(); j++) {
      oss << " ";
    }
    oss << std::endl;
  }
  for (size_t i = tir.size(); i < lines.size(); i++) {
    oss << lines[i] << std::endl;
  }

  return oss.str();
}

llvm::json::Object MetaData::serialize() const {
  llvm::json::Object root;

  root["recoverFlag"] = recoverFlag;
  root["recoverReason"] = recoverReason;

  root["currentPath"] = currentPath;
  root["originalCommand"] = originalCommand;
  root["inputPath"] = inputPath;
  root["outputPath"] = outputPath;

  root["totalTimeMs"] = totalTimeMs;
  root["frontTimeMs"] = frontTimeMs;
  root["backTimeMs"] = backTimeMs;

  return root;
}

void MetaData::deserialize(llvm::json::Object &root) {
  recoverFlag = root["recoverFlag"].getAsBoolean().value();
  recoverReason = root["recoverReason"].getAsString().value();

  currentPath = root["currentPath"].getAsString().value().str();
  originalCommand = root["originalCommand"].getAsString().value().str();
  inputPath = root["inputPath"].getAsString().value().str();
  outputPath = root["outputPath"].getAsString().value().str();

  totalTimeMs = root["totalTimeMs"].getAsInteger().value();
  frontTimeMs = root["frontTimeMs"].getAsInteger().value();
  backTimeMs = root["backTimeMs"].getAsInteger().value();
}

llvm::json::Object IncMetaData::serialize() const {
  auto root = MetaData::serialize();

  root["incFlag"] = incFlag;
  root["cannotIncReason"] = cannotIncReason;

  root["funcXTime"] = funcXTime;
  root["funcVTime"] = funcVTime;

  llvm::json::Array arr;
  for (const auto &line : topIncludeRegion) {
    arr.push_back(line);
  }
  root["topIncludeRegion"] = llvm::json::Value(std::move(arr));

  llvm::json::Object sub;
  for (auto &kv : headerTs) {
    sub[kv.first] = kv.second;
  }
  root["headerTs"] = llvm::json::Value(std::move(sub));

  return root;
}

void IncMetaData::deserialize(llvm::json::Object &root) {
  MetaData::deserialize(root);

  incFlag = root["recoverFlag"].getAsBoolean().value();
  cannotIncReason = root["cannotIncReason"].getAsString().value();

  funcXTime = root["funcXTime"].getAsInteger().value();
  funcVTime = root["funcVTime"].getAsInteger().value();

  auto *arr = root["topIncludeRegion"].getAsArray();
  ILLVM_FCHECK(arr != nullptr,
                    "Failed to parse JSON: Can not convert topIncludeRegion to "
                    "json array");

  topIncludeRegion.clear();
  for (const auto &line : *arr) {
    topIncludeRegion.push_back(line.getAsString().value().str());
  }

  auto *headerTsObj = root["headerTs"].getAsObject();
  ILLVM_FCHECK(
      headerTsObj != nullptr,
      "Failed to parse JSON: Can not convert headerTs to json object");

  headerTs.clear();
  for (const auto &kv : *headerTsObj) {
    const std::string key = kv.first.str();
    headerTs[key] = kv.second.getAsInteger().value();
  }
}

llvm::json::Object IncLineCheckMetaData::serialize() const {
  auto root = MetaData::serialize();

  root["hashHashFlag"] = hashHashFlag;
  root["baseFuncDefNum"] = baseFuncDefNum;

  llvm::json::Array arr;
  for (const auto &macro : inValidMacro) {
    arr.push_back(macro);
  }
  root["inValidMacro"] = llvm::json::Value(std::move(arr));

  return root;
}

void IncLineCheckMetaData::deserialize(llvm::json::Object &root) {
  MetaData::deserialize(root);

  hashHashFlag = root["hashHashFlag"].getAsBoolean().value();
  baseFuncDefNum = root["baseFuncDefNum"].getAsInteger().value();

  auto *arr = root["inValidMacro"].getAsArray();
  ILLVM_FCHECK(arr != nullptr,
                    "Failed to parse JSON: Can not convert inValidMacro to "
                    "json array");

  inValidMacro.clear();
  for (const auto &macro : *arr) {
    inValidMacro.insert(macro.getAsString().value().str());
  }
}

llvm::json::Object ShareCheckMetaData::serialize() const {
  auto root = MetaData::serialize();

  root["originalTimeMs"] = originalTimeMs;
  root["masterTimeMs"] = masterTimeMs;
  root["clientTimeMs"] = clientTimeMs;
  root["originalPPLoc"] = originalPPLoc;
  root["funcXedPPLoc"] = funcXedPPLoc;

  return root;
}

void ShareCheckMetaData::deserialize(llvm::json::Object &root) {
  MetaData::deserialize(root);

  originalTimeMs = root["originalTimeMs"].getAsInteger().value();
  masterTimeMs = root["masterTimeMs"].getAsInteger().value();
  clientTimeMs = root["clientTimeMs"].getAsInteger().value();
  originalPPLoc = root["originalPPLoc"].getAsInteger().value();
  funcXedPPLoc = root["funcXedPPLoc"].getAsInteger().value();
}

llvm::json::Object LineMacroCheckMetaData::serialize() const {
  auto root = MetaData::serialize();

  root["totalFuncNum"] = totalFuncNum;
  root["funcWithLineMacroNum"] = funcWithLineMacroNum;

  return root;
}

void LineMacroCheckMetaData::deserialize(llvm::json::Object &root) {
  MetaData::deserialize(root);

  totalFuncNum = root["totalFuncNum"].getAsInteger().value();
  funcWithLineMacroNum = root["funcWithLineMacroNum"].getAsInteger().value();
}

} // namespace iclang