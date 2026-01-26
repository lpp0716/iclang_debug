set( LLVM_LINK_COMPONENTS
    ${LLVM_TARGETS_TO_BUILD}
    Analysis
    CodeGen
    Core
    IPO
    AggressiveInstCombine
    InstCombine
    Instrumentation
    MC
    MCParser
    ObjCARCOpts
    Option
    ScalarOpts
    Support
    TransformUtils
    Vectorize
)

add_clang_tool(iclang-funcv
    main.cpp

    DEPENDS
    intrinsics_gen
)

clang_target_link_libraries(iclang-funcv
    PRIVATE
    clangBasic
    clangCodeGen
    clangDriver
    clangFrontend
    clangFrontendTool
    clangSerialization
)

