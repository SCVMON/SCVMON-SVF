
if(DEFINED IN_SOURCE_BUILD)
    set(LLVM_LINK_COMPONENTS BitWriter Core IPO IrReader InstCombine Instrumentation Target Linker Analysis ScalarOpts Support Svf Cudd)
    add_llvm_tool( srcsnka srcsnka.cpp )
else()
    add_executable( srcsnka srcsnka.cpp )

    target_link_libraries( srcsnka Svf Cudd ${llvm_libs} )

    set_target_properties( srcsnka PROPERTIES
                           RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
endif()
