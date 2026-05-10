add_llvm_component_library(LLVMILLVMSupport
  BinFile.cpp
  Diagnostics.cpp
  FileSystem.cpp
  Interval.cpp
  Logger.cpp
  Strings.cpp
  Time.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/illvm/Support

  LINK_COMPONENTS
  Support

  DEPENDS
  intrinsics_gen
  )
