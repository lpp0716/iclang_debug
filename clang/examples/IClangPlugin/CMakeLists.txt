add_llvm_library(IclangPlugin MODULE IClangPlugin.cpp PLUGIN_TOOL clang)

if(WIN32 OR CYGWIN)
  set(LLVM_LINK_COMPONENTS
    Support
  )
  clang_target_link_libraries(IClangPlugin PRIVATE
    clangAST
    clangBasic
    clangFrontend
    ClangLex
  )
endif()