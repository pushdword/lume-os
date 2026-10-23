set(LLVM_LINK_COMPONENTS AArch64CodeGen AArch64AsmParser AArch64Desc AArch64Info Core Support TargetParser Option)
add_llvm_executable(lumec main.cpp PARTIAL_SOURCES_INTENDED)
target_sources(lumec PRIVATE ${CMAKE_SOURCE_DIR}/../clang/tools/driver/cc1as_main.cpp)
target_include_directories(lumec PRIVATE
    ${CMAKE_SOURCE_DIR}/../clang/include
    ${CMAKE_BINARY_DIR}/tools/clang/include
    ${CMAKE_SOURCE_DIR}/../lld/include
    ${LUME_OS_ROOT}/include/common
    ${LUME_OS_ROOT}/include/common/posix
    ${LUME_KERNEL_ROOT}/include/uapi/common ${LUME_KERNEL_ROOT}/include/uapi/aarch64)
target_link_libraries(lumec PRIVATE clangFrontendTool clangFrontend clangCodeGen
    clangDriver clangSerialization clangSema clangParse clangLex clangAST clangBasic
    lldELF lldCommon)

set(lume_basic_target clangBasic)
if(TARGET obj.clangBasic)
    set(lume_basic_target obj.clangBasic)
endif()
get_target_property(lume_basic_sources ${lume_basic_target} SOURCES)
list(FILTER lume_basic_sources EXCLUDE REGEX "(^|/)Targets\\.cpp$")
set_property(TARGET ${lume_basic_target} PROPERTY SOURCES "${lume_basic_sources}")
target_sources(${lume_basic_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../support/Targets.cpp)
target_include_directories(${lume_basic_target} PRIVATE ${CMAKE_SOURCE_DIR}/../clang/lib/Basic)

function(lume_replace_source target original replacement include_directory)
    if(TARGET obj.${target})
        set(target obj.${target})
    endif()
    get_target_property(sources ${target} SOURCES)
    list(FILTER sources EXCLUDE REGEX "(^|/)${original}$")
    set_property(TARGET ${target} PROPERTY SOURCES "${sources}")
    target_sources(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../support/${replacement})
    target_include_directories(${target} PRIVATE ${include_directory})
endfunction()

lume_replace_source(clangCodeGen CodeGenModule.cpp CodeGenModule.cpp
    ${CMAKE_SOURCE_DIR}/../clang/lib/CodeGen)
lume_replace_source(lldELF Target.cpp ELFTarget.cpp ${CMAKE_SOURCE_DIR}/../lld/ELF)
target_compile_definitions(lumec PRIVATE LUME_ARCH_AARCH64=1)
set(LUME_NATIVE_PREFIX "/usr/lib/clang" CACHE STRING "Native compiler sysroot")
target_compile_definitions(lumec PRIVATE LUME_NATIVE_PREFIX="${LUME_NATIVE_PREFIX}")

if(LUME_SUPPORT_PATH_SOURCE)
    get_filename_component(lume_support_directory ${LUME_SUPPORT_PATH_SOURCE} DIRECTORY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../support/Path.inc
        ${lume_support_directory}/Lume/Path.inc COPYONLY)
    get_target_property(lume_support_sources LLVMSupport SOURCES)
    list(FILTER lume_support_sources EXCLUDE REGEX "(^|/)Path\\.cpp$")
    set_property(TARGET LLVMSupport PROPERTY SOURCES "${lume_support_sources}")
    target_sources(LLVMSupport PRIVATE ${LUME_SUPPORT_PATH_SOURCE})
    target_include_directories(LLVMSupport PRIVATE ${CMAKE_SOURCE_DIR}/lib/Support
        ${LUME_OS_ROOT}/include/common ${LUME_KERNEL_ROOT}/include/uapi/common
        ${LUME_KERNEL_ROOT}/include/uapi/aarch64)
    target_compile_definitions(LLVMSupport PRIVATE LUME_ARCH_AARCH64=1)
endif()

enable_language(ASM)
add_library(lume_native_crt OBJECT
    ${LUME_OS_ROOT}/arch/aarch64/start.S
    ${LUME_OS_ROOT}/libc/aarch64/setjmp.S
    ${LUME_OS_ROOT}/libc/aarch64/float.c
    ../cxxabi.cpp ../thread_compat.cpp
    ${LUME_OS_ROOT}/libc/common/api.c
    ${LUME_OS_ROOT}/libc/aarch64/api.c
    ${LUME_OS_ROOT}/libc/aarch64/thread_backend.c
    ${LUME_OS_ROOT}/libc/common/posix/pthread.c
    ${LUME_OS_ROOT}/libc/common/posix/posix_runtime.c
    ${LUME_OS_ROOT}/libc/common/posix/errno_map.c
    ${LUME_OS_ROOT}/libc/common/posix/sbrk.c
    ${LUME_OS_ROOT}/libc/common/posix/directory.c
    ${LUME_OS_ROOT}/libc/common/posix/filesystem.c
    ${LUME_OS_ROOT}/libc/common/posix/process.c
    ${LUME_OS_ROOT}/libc/common/posix/fd.c
    ${LUME_OS_ROOT}/libc/common/posix/time.c)
target_include_directories(lume_native_crt PRIVATE
    ../include ${LUME_OS_ROOT}/include/common ${LUME_OS_ROOT}/include/common/posix
    ${LUME_KERNEL_ROOT}/include/uapi/common ${LUME_KERNEL_ROOT}/include/uapi/aarch64)
target_compile_definitions(lume_native_crt PRIVATE LUME_ARCH_AARCH64=1 LUME_NATIVE_CXX=1
    _GNU_SOURCE _DEFAULT_SOURCE _POSIX_C_SOURCE=200809L _POSIX_MONOTONIC_CLOCK=200809L)
target_sources(lumec PRIVATE $<TARGET_OBJECTS:lume_native_crt>)
target_link_options(lumec PRIVATE -nostdlib -static
    -Wl,--gc-sections,--icf=safe,--build-id=none,--strip-all,-z,max-page-size=4096
    -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/../native.ld)
target_link_libraries(lumec PRIVATE ${LUME_CXX_ROOT}/lib/libc++.a
    ${CMAKE_SYSROOT}/lib/libc.a ${LUME_BUILTINS})

add_llvm_executable(native-runtime ${LUME_OS_ROOT}/tests/aarch64/native_runtime.cpp PARTIAL_SOURCES_INTENDED)
add_executable(lume-tool launcher.c)
target_include_directories(lume-tool PRIVATE ${LUME_OS_ROOT}/include/common
    ${LUME_OS_ROOT}/include/common/posix ${LUME_KERNEL_ROOT}/include/uapi/common
    ${LUME_KERNEL_ROOT}/include/uapi/aarch64)
target_compile_definitions(lume-tool PRIVATE LUME_ARCH_AARCH64=1)
set(LLVM_LINK_COMPONENTS Support Option TargetParser)
add_llvm_executable(lumeld lld.cpp PARTIAL_SOURCES_INTENDED)
target_include_directories(lumeld PRIVATE ${CMAKE_SOURCE_DIR}/../lld/include)
target_link_libraries(lumeld PRIVATE lldELF lldCommon)
foreach(lume_tool llvm-ar llvm-objcopy llvm-nm llvm-readobj native-runtime lumeld lume-tool)
    target_sources(${lume_tool} PRIVATE $<TARGET_OBJECTS:lume_native_crt>)
    target_link_options(${lume_tool} PRIVATE -nostdlib -static
        -Wl,--gc-sections,--icf=safe,--build-id=none,--strip-all,-z,max-page-size=4096
        -Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/../native.ld)
    target_link_libraries(${lume_tool} PRIVATE ${LUME_CXX_ROOT}/lib/libc++.a
        ${CMAKE_SYSROOT}/lib/libc.a ${LUME_BUILTINS})
endforeach()
target_sources(llvm-ar PRIVATE tool_entry.cpp)
target_sources(llvm-objcopy PRIVATE tool_entry.cpp)
target_compile_definitions(llvm-ar PRIVATE LUME_TOOL_MAIN=llvm_ar_main)
target_compile_definitions(llvm-objcopy PRIVATE LUME_TOOL_MAIN=llvm_objcopy_main)
target_sources(llvm-nm PRIVATE tool_entry.cpp)
target_compile_definitions(llvm-nm PRIVATE LUME_TOOL_MAIN=llvm_nm_main)
target_sources(llvm-readobj PRIVATE tool_entry.cpp)
target_compile_definitions(llvm-readobj PRIVATE LUME_TOOL_MAIN=llvm_readobj_main)
