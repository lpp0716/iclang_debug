set(LLVM_LINK_COMPONENTS
  Option
  ProfileData
  ILLVMSupport
  )

add_clang_library(clangIClangASTSupport
  ASTGlobal.cpp
  ASTMetaData.cpp

  DEPENDS
  ClangDriverOptions

  LINK_LIBS
  clangBasic
  clangIClangSupport
  ${system_libs}
)