set(LLVM_LINK_COMPONENTS
  Option
  ProfileData
  Support
  ILLVMSupport
  ILLVMFuncVELF
  )

add_clang_library(clangIClangDriver
  CheckDriver.cpp
  Driver.cpp
  DriverBase.cpp
  IncDriver.cpp
  ShareDriver.cpp

  DEPENDS
  ClangDriverOptions

  LINK_LIBS
  clangBasic
  clangIClangSupport
  ${system_libs}
  )
