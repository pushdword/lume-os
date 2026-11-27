set(LLVM_LINK_COMPONENTS AArch64CodeGen AArch64AsmParser AArch64Desc AArch64Info Core Support TargetParser Option)
add_llvm_executable(lumec main.cpp PARTIAL_SOURCES_INTENDED)
target_sources(lumec PRIVATE ${CMAKE_SOURCE_DIR}/../clang/tools/driver/cc1as_main.cpp)
target_include_directories(lumec PRIVATE
    ${CMAKE_SOURCE_DIR}/../clang/include
    ${CMAKE_BINARY_DIR}/tools/clang/include
    ${CMAKE_SOURCE_DIR}/../lld/include
    ${LUME_OS_ROOT}/arch/aarch64/include
    ${LUME_OS_ROOT}/include/lume
    ${LUME_OS_ROOT}/include/posix
    ${LUME_KERNEL_ROOT}/include/uapi ${LUME_KERNEL_ROOT}/include/abi/native)
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
        ${LUME_OS_ROOT}/arch/aarch64/include
    ${LUME_OS_ROOT}/include/lume ${LUME_KERNEL_ROOT}/include/uapi
        ${LUME_KERNEL_ROOT}/include/abi/native)
    target_compile_definitions(LLVMSupport PRIVATE LUME_ARCH_AARCH64=1)
endif()

set(LUME_NATIVE_SDK "${CMAKE_SYSROOT}" CACHE PATH "Lume native SDK")
set(LUME_BUILTINS "${LUME_NATIVE_SDK}/lib/libcompiler-rt.a" CACHE FILEPATH "Compiler runtime archive")
set(lume_sdk_crt "${LUME_NATIVE_SDK}/lib/crt0-cxx.o")
set(lume_sdk_libraries)
foreach(archive liblume.a libc++.a libc++abi.a libunwind.a libc.a)
    list(APPEND lume_sdk_libraries "${LUME_NATIVE_SDK}/lib/${archive}")
endforeach()
list(APPEND lume_sdk_libraries "${LUME_BUILTINS}")
foreach(input IN ITEMS "${lume_sdk_crt}" "${LUME_NATIVE_SDK}/native.ld" ${lume_sdk_libraries})
    if(NOT EXISTS "${input}")
        message(FATAL_ERROR "Missing Lume native SDK input: ${input}")
    endif()
endforeach()
set_source_files_properties("${lume_sdk_crt}" PROPERTIES EXTERNAL_OBJECT TRUE)

set(LLVM_LINK_COMPONENTS)
add_llvm_executable(native-runtime ${LUME_OS_ROOT}/tests/integration/native_runtime.cpp PARTIAL_SOURCES_INTENDED)
add_executable(lume-tool launcher.c)
target_include_directories(lume-tool PRIVATE ${LUME_OS_ROOT}/arch/aarch64/include
    ${LUME_OS_ROOT}/include/lume
    ${LUME_OS_ROOT}/include/posix ${LUME_KERNEL_ROOT}/include/uapi
    ${LUME_KERNEL_ROOT}/include/abi/native)
target_compile_definitions(lume-tool PRIVATE LUME_ARCH_AARCH64=1)
set(LLVM_LINK_COMPONENTS Support Option TargetParser)
add_llvm_executable(lumeld lld.cpp PARTIAL_SOURCES_INTENDED)
target_include_directories(lumeld PRIVATE ${CMAKE_SOURCE_DIR}/../lld/include)
target_link_libraries(lumeld PRIVATE lldELF lldCommon)
foreach(lume_tool lumec llvm-ar llvm-objcopy llvm-nm llvm-readobj native-runtime lumeld lume-tool)
    target_sources(${lume_tool} PRIVATE "${lume_sdk_crt}")
    target_link_options(${lume_tool} PRIVATE -nostdlib -static
        -Wl,--gc-sections,--icf=safe,--build-id=none,--strip-all,-z,max-page-size=4096
        "-Wl,-T,${LUME_NATIVE_SDK}/native.ld")
    set_property(TARGET ${lume_tool} APPEND PROPERTY LINK_DEPENDS "${LUME_NATIVE_SDK}/native.ld")
    target_link_libraries(${lume_tool} PRIVATE -Wl,--start-group ${lume_sdk_libraries}
        -Wl,--end-group)
endforeach()
target_sources(llvm-ar PRIVATE tool_entry.cpp)
target_sources(llvm-objcopy PRIVATE tool_entry.cpp)
target_compile_definitions(llvm-ar PRIVATE LUME_TOOL_MAIN=llvm_ar_main)
target_compile_definitions(llvm-objcopy PRIVATE LUME_TOOL_MAIN=llvm_objcopy_main)
target_sources(llvm-nm PRIVATE tool_entry.cpp)
target_compile_definitions(llvm-nm PRIVATE LUME_TOOL_MAIN=llvm_nm_main)
target_sources(llvm-readobj PRIVATE tool_entry.cpp)
target_compile_definitions(llvm-readobj PRIVATE LUME_TOOL_MAIN=llvm_readobj_main)
