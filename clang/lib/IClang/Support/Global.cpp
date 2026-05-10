#include "iclang/Support/Global.h"

#include "illvm/Support/Diagnostics.h"
#include "illvm/Support/FileSystem.h"

#include "llvm/Support/JSON.h"

namespace iclang {

void Global::saveMetaDataToFile(const std::string &filepath,
                                const illvm::BPtr<MetaData> &metaData) {
  const auto rootValue = llvm::json::Value(metaData->serialize());
  const auto content = llvm::formatv("{0:2}", rootValue).str();
  illvm::FileSystem::saveStr(filepath, content);
}

illvm::OPtr<MetaData>
Global::loadMetaDataFromFile(const std::string &filepath,
                             const IClangMode iClangMode) {
  const auto jsonData = illvm::FileSystem::readAll(filepath);
  auto valueOrErr = llvm::json::parse(jsonData);
  ILLVM_FATAL_ON(valueOrErr.takeError(), "Can not parse meta data: " + filepath);
  auto *rootPtr = valueOrErr->getAsObject();
  ILLVM_FCHECK(rootPtr != nullptr, "Can not load object from meta data: " + filepath);

  auto root = std::move(*rootPtr);

  auto metaData = createMetaData(iClangMode);
  metaData->deserialize(root);

  return metaData;
}

} // namespace iclang
