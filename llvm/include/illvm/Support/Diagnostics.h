#ifndef ILLVM_DIAGNOSTICS_H
#define ILLVM_DIAGNOSTICS_H

#include <cassert>

#include "llvm/Support/Error.h"
#include "llvm/Support/FormatVariadic.h"

#include "illvm/Support/Logger.h"

namespace illvm {

class ILLVMError : public llvm::ErrorInfo<ILLVMError> {
public:
  static char ID;

  explicit ILLVMError(std::string _msg) : msg(std::move(_msg)) {}

  void log(llvm::raw_ostream &OS) const override { OS << msg; }

  std::error_code convertToErrorCode() const override {
    return llvm::inconvertibleErrorCode();
  }

private:
  std::string msg;
};

#define ILLVM_ETRANS(RERR)                                                     \
  if (auto err = RERR; __builtin_expect(static_cast<bool>(err), 0)) {          \
    return err;                                                                \
  }

#define ILLVM_ECHECK(EXPR, MSG)                                                \
  if (__builtin_expect(!(EXPR), 0)) {                                          \
    return llvm::make_error<illvm::ILLVMError>(                                \
        llvm::formatv("{0}: Check failed: `{1}`: {2}", __PRETTY_FUNCTION__,    \
                      #EXPR, MSG)                                              \
            .str());                                                           \
  }

#define ILLVM_ECHECK_TO(EXPR, MSG, ERR)                                        \
  if (__builtin_expect(!(EXPR), 0)) {                                          \
    ERR = llvm::make_error<illvm::ILLVMError>(                                 \
        llvm::formatv("{0}: Check failed: `{1}`: {2}", __PRETTY_FUNCTION__,    \
                      #EXPR, MSG)                                              \
            .str());                                                           \
    return;                                                                    \
  }

#define ILLVM_FCHECK(EXPR, MSG)                                                \
  if (__builtin_expect(!(EXPR), 0)) {                                          \
    illvm::Logger::getInstance().fatal(                                        \
        __PRETTY_FUNCTION__,                                                   \
        llvm::formatv("Check failed: `{0}`: {1}", #EXPR, MSG).str());          \
  }

#define ILLVM_FATAL_ON(RERR, MSG)                                              \
  llvm::handleAllErrors(RERR, [&](const llvm::ErrorInfoBase &errInfo) {        \
    illvm::Logger::getInstance().fatal(                                        \
        __PRETTY_FUNCTION__,                                                   \
        llvm::formatv("{0}: {1}", MSG, errInfo.message()).str());              \
  });

} // namespace illvm

#endif // ILLVM_DIAGNOSTICS_H
