cmake_minimum_required(VERSION 3.19)
project(loop_unrolling_bench LANGUAGES CXX)

set(DEFAULT_ITERS   100        CACHE STRING "Repetitions per timing run")
set(UNROLL_FACTORS  1 2 4 8    CACHE STRING "Space-separated unroll factors")

set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-march=native -Wall -Wextra -Werror)
endif()

function(add_copy_target OPT UF)
    set(target copy_${OPT}_u${UF})

    add_executable(${target} code.cpp)

    target_compile_definitions(${target}
        PRIVATE
            UF=${UF}
            ITERS=${DEFAULT_ITERS})

    target_compile_options(${target} PRIVATE ${OPT})

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target}
            PRIVATE -Wno-error=unknown-pragmas)
    endif()
endfunction()

foreach(uf ${UNROLL_FACTORS})
    add_copy_target(-O0 ${uf})
    add_copy_target(-O1 ${uf})
    add_copy_target(-O2 ${uf})
    add_copy_target(-O3 ${uf})
endforeach()
