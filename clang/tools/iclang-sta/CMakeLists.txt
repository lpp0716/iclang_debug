set(LLVM_LINK_COMPONENTS
  Support
  ILLVMSupport
  )

add_clang_tool(iclang-sta
  main.cpp
  )

clang_target_link_libraries(iclang-sta
  PRIVATE
  clangBasic
  clangIClangSupport
  clangIClangASTSupport
  )