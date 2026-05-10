add_llvm_component_library(LLVMILLVMFuncVELF
  EhFrame.cpp
  EHFrameSection.cpp
  FuncV.cpp
  GroupSection.cpp
  NormalSection.cpp
  ObjFile.cpp
  Reference.cpp
  Relocation.cpp
  RelocationSection.cpp
  Section.cpp
  StringTableSection.cpp
  Symbol.cpp
  SymbolTableSection.cpp
  Type.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/illvm/FuncV/ELF

  LINK_COMPONENTS
  Support
  ILLVMSupport
  BinaryFormat

  DEPENDS
  intrinsics_gen
  )
