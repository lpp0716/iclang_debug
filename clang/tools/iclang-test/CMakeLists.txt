set(LLVM_LINK_COMPONENTS
  Support
  ILLVMSupport
  )

add_clang_tool(iclang-test
  main.cpp
  )

clang_target_link_libraries(iclang-test
  PRIVATE
  clangBasic
  clangIClangSupport
  clangIClangASTSupport
  )