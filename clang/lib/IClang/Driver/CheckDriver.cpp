#include "iclang/Driver/CheckDriver.h"

namespace iclang {

int IncLineCheckDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::IncLineCheckMode);
  return DriverBase::runBase(global, originalArgv, clangDriver);
}

int LineMacroCheckDriver::run(
    Global &global, const llvm::SmallVector<const char *, 128> &originalArgv,
    const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::LineMacroCheckMode);
  return DriverBase::runBase(global, originalArgv, clangDriver);
}

int DumpDriver::run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::DumpMode);
  return DriverBase::runBase(global, originalArgv, clangDriver);
}

int ProfileDriver::run(Global &global,
                 const llvm::SmallVector<const char *, 128> &originalArgv,
                 const clang::driver::Driver &clangDriver) {
  assert(global.getIClangMode() == IClangMode::ProfileMode);
  return DriverBase::runBase(global, originalArgv, clangDriver);
}

} // namespace iclang