cmake_minimum_required(VERSION 3.6)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")
project(rwrapper)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED)

if(NOT DEFINED GRPC_DIST)
    set(GRPC_DIST ${CMAKE_HOME_DIRECTORY}/grpc)
endif()

if(NOT DEFINED R_HOME AND DEFINED ENV{R_HOME})
    set(R_HOME $ENV{R_HOME})
endif()
if(NOT DEFINED R_HOME)
    execute_process(
            COMMAND R RHOME
            OUTPUT_VARIABLE R_HOME
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE STATUS
    )
    if(STATUS AND NOT STATUS EQUAL 0)
        message("R HOME invocation failed: ${STATUS}")
    endif()
endif()
message("R_HOME is set to '${R_HOME}'")

if(NOT DEFINED R_INCLUDE AND DEFINED ENV{R_INCLUDE})
    set(R_INCLUDE $ENV{R_INCLUDE})
endif()
if(NOT DEFINED R_INCLUDE)
    if(WIN32)
        string(REPLACE "\"" "" R_HOME ${R_HOME})
        message("${R_HOME}/bin/x64/R --slave -e \"cat\(R.home\('include'\)\)\"")
        execute_process(
                COMMAND ${R_HOME}/bin/x64/R --slave -e "cat\(R.home\('include'\)\)"
                OUTPUT_VARIABLE R_INCLUDE
                RESULT_VARIABLE STATUS
        )
        if(STATUS AND NOT STATUS EQUAL 0)
            message("R invocation failed: ${STATUS}")
        endif()
    else()
        execute_process(
                COMMAND ${R_HOME}/bin/R --slave -e cat\(R.home\('include'\)\)
                OUTPUT_VARIABLE R_INCLUDE
                RESULT_VARIABLE STATUS
        )
        if(STATUS AND NOT STATUS EQUAL 0)
            message("R invocation failed: ${STATUS}")
        endif()
    endif()
endif()
message("R_INCLUDE is set to ${R_INCLUDE}")

list(APPEND RWRAPPER_SOURCES
        src/IO.cpp
        src/RPIServiceImpl.cpp
        src/CppExports.cpp
        src/graphics/Evaluator.cpp
        src/graphics/Interface.cpp
        src/graphics/InitHelper.cpp
        src/graphics/MasterDevice.cpp
        src/graphics/DeviceManager.cpp
        src/graphics/DeviceSlotLock.cpp
        src/graphics/FontUtil.cpp
        src/graphics/PlotUtil.cpp
        src/graphics/ScopeProtector.cpp
        src/graphics/SlaveDevice.cpp
        src/graphics/SnapshotUtil.cpp
        src/graphics/REagerGraphicsDevice.cpp
        src/base64/base64.cpp
        src/base64/base64r.cpp
        src/RPIServiceMethods.cpp
        src/RRefs.cpp
        src/ExecuteCode.cpp
        src/RLoader.cpp
        src/DataFrame.cpp
        src/Options.cpp
        src/debugger/SourceFileManager.cpp
        src/debugger/RDebugger.cpp
        src/debugger/DebuggerMethods.cpp
        src/debugger/TextBuilder.cpp
        src/RInternals/RInternals.cpp
        src/HTMLViewer.cpp
        src/Subprocess.cpp
        src/Init.cpp
        src/RStuff/MySEXP.cpp
        src/RStuff/Conversion.cpp
        src/Session.cpp
        src/EventLoop.cpp
        src/CrashReport.cpp
        src/RInternals/MatchArgs.cpp
        src/RStudioApi.cpp
        src/Timer.cpp
        src/RStuff/RInterrupt.cpp)

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_WIN32_WINNT=0x600)
    add_definitions(-DWin32)
    if(DEFINED MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /stack:0x1000000")
endif()

if(APPLE)
    add_definitions(-D_MACOS)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DRWRAPPER_DEBUG")

if(DEFINED CRASHPAD_DIR)
    message("Crashpad is enabled")
    add_definitions(-DCRASHPAD_ENABLED)

    set(CRASHPAD_INCLUDE_DIR "${CRASHPAD_DIR}")
    set(CRASHPAD_INNER_DIR "${CRASHPAD_DIR}/crashpad")
    include_directories(SYSTEM ${CRASHPAD_INCLUDE_DIR})
    include_directories(SYSTEM ${CRASHPAD_INNER_DIR})
    include_directories(SYSTEM "${CRASHPAD_INNER_DIR}/third_party/mini_chromium/mini_chromium")
    set(CRASHPAD_OUTPUT_DIR "${CRASHPAD_INNER_DIR}/out/Default")
    set(CRASHPAD_LIBRARY_DIR "${CRASHPAD_OUTPUT_DIR}/obj")
    list(APPEND RWRAPPER_SOURCES src/CrashpadHandler.cpp)

    if(APPLE)
        find_library(APPLICATION_SERVICES_LIBRARY NAMES ApplicationServices)
        list(APPEND CRASHPAD_LIBRARIES ${APPLICATION_SERVICES_LIBRARY})
        find_library(CORE_FOUNDATION_LIBRARY NAMES CoreFoundation)
        list(APPEND CRASHPAD_LIBRARIES ${CORE_FOUNDATION_LIBRARY})
        find_library(FOUNDATION_LIBRARY NAMES Foundation)
        list(APPEND CRASHPAD_LIBRARIES ${FOUNDATION_LIBRARY})
        find_library(SECURITY_LIBRARY NAMES Security)
        list(APPEND CRASHPAD_LIBRARIES ${SECURITY_LIBRARY})
        find_library(IOKIT_LIBRARY NAMES IOKit)
        list(APPEND CRASHPAD_LIBRARIES ${IOKIT_LIBRARY})
        find_library(BSM_LIBRARY NAMES bsm)
        list(APPEND CRASHPAD_LIBRARIES ${BSM_LIBRARY})
    endif()

    file(GLOB_RECURSE CRASHPAD_LIBCLIENT "${CRASHPAD_LIBRARY_DIR}/*client${CMAKE_STATIC_LIBRARY_SUFFIX}")
    file(GLOB_RECURSE CRASHPAD_LIBUTIL "${CRASHPAD_LIBRARY_DIR}/*util${CMAKE_STATIC_LIBRARY_SUFFIX}")
    file(GLOB_RECURSE CRASHPAD_LIBBASE "${CRASHPAD_LIBRARY_DIR}/*base${CMAKE_STATIC_LIBRARY_SUFFIX}")
    file(GLOB_RECURSE CRASHPAD_LIBCOMPAT "${CRASHPAD_LIBRARY_DIR}/*compat${CMAKE_STATIC_LIBRARY_SUFFIX}")
    list(APPEND CRASHPAD_LIBRARIES ${CRASHPAD_LIBCLIENT} ${CRASHPAD_LIBUTIL} ${CRASHPAD_LIBBASE} ${CRASHPAD_LIBCOMPAT})
endif()


set(_PROTOBUF_PROTOC ${GRPC_DIST}/tools/protobuf/protoc${CMAKE_EXECUTABLE_SUFFIX})
set(_GRPC_CPP_PLUGIN_EXECUTABLE ${GRPC_DIST}/tools/grpc/grpc_cpp_plugin${CMAKE_EXECUTABLE_SUFFIX})
set(service_proto "${CMAKE_HOME_DIRECTORY}/Rkernel-proto/service.proto")
get_filename_component(service_proto_path "${service_proto}" DIRECTORY)
set(service_proto_srcs "${CMAKE_HOME_DIRECTORY}/gens/protos/service.pb.cc")
set(service_proto_hdrs "${CMAKE_HOME_DIRECTORY}/gens/protos/service.pb.h")
set(service_grpc_srcs "${CMAKE_HOME_DIRECTORY}/gens/protos/service.grpc.pb.cc")
set(service_grpc_hdrs "${CMAKE_HOME_DIRECTORY}/gens/protos/service.grpc.pb.h")

set(classes_proto "${CMAKE_HOME_DIRECTORY}/Rkernel-proto/classes.proto")
get_filename_component(classes_proto_path "${classes_proto}" DIRECTORY)
set(classes_proto_srcs "${CMAKE_HOME_DIRECTORY}/gens/protos/classes.pb.cc")
set(classes_proto_hdrs "${CMAKE_HOME_DIRECTORY}/gens/protos/classes.pb.h")
set(classes_grpc_srcs "${CMAKE_HOME_DIRECTORY}/gens/protos/classes.grpc.pb.cc")
set(classes_grpc_hdrs "${CMAKE_HOME_DIRECTORY}/gens/protos/classes.grpc.pb.h")
add_custom_command(
        OUTPUT "${service_proto_srcs}" "${service_proto_hdrs}" "${service_grpc_srcs}" "${service_grpc_hdrs}"
        "${classes_proto_srcs}" "${classes_proto_hdrs}" "${classes_grpc_srcs}" "${classes_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_HOME_DIRECTORY}/gens/protos"
        --cpp_out "${CMAKE_HOME_DIRECTORY}/gens/protos"
        -I "${service_proto_path}"
        -I "${classes_proto_path}"
        -I "${GRPC_DIST}/include"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${service_proto}"
        "${classes_proto}"
        DEPENDS "${service_proto}" "${classes_proto}")
include_directories("${CMAKE_HOME_DIRECTORY}/gens")
include_directories("${R_INCLUDE}")
include_directories("${GRPC_DIST}/include")

if(UNIX)
    link_directories("${R_HOME}/lib")
    set(RWRAPPER_MAIN src/main_unix.cpp)
endif()
if(WIN32)
    link_directories("${R_HOME}/bin/x64")
    set(RWRAPPER_MAIN src/main_win.cpp)
endif()


add_definitions(-DR_INTERFACE_PTRS)

set(CMAKE_PREFIX_PATH "${GRPC_DIST}")
set(OPENSSL_ROOT_DIR "${GRPC_DIST}")
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(tiny-process-library CONFIG REQUIRED NAMES tiny-process-library unofficial-tiny-process-library)
get_filename_component(tiny-process-library_dir_name "${tiny-process-library_DIR}" NAME)

if(UNIX)
    set(SYSTEM_SPECIFIC_SOURCES src/UnixForkHandler.cpp src/SubprocessUnix.cpp src/EventLoopUnix.cpp)
else()
    set(SYSTEM_SPECIFIC_SOURCES src/SubprocessWin.cpp src/EventLoopWin.cpp)
endif()


add_executable(
        rwrapper
        ${RWRAPPER_MAIN}
        ${RWRAPPER_SOURCES}
        ${SYSTEM_SPECIFIC_SOURCES}
        ${service_proto_srcs}
        ${service_grpc_srcs}
        ${classes_proto_srcs}
        ${classes_grpc_srcs}
)

if(DEFINED CRASHPAD_DIR)
    target_link_libraries(rwrapper ${CRASHPAD_LIBRARIES})
endif()

target_link_libraries(rwrapper gRPC::gpr gRPC::grpc gRPC::grpc++)
target_link_libraries(rwrapper c-ares::cares)

if(UNIX)
    if(APPLE)
        target_link_libraries(rwrapper "-framework Foundation")
        target_link_libraries(rwrapper resolv)
    else()
        target_link_libraries(rwrapper pthread dl)
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif()

if(WIN32)
    target_link_libraries(rwrapper WS2_32)
    target_link_libraries(rwrapper Rgraphapp)
endif()

target_link_libraries(rwrapper R)
if(tiny-process-library_dir_name STREQUAL unofficial-tiny-process-library)
    target_link_libraries(rwrapper unofficial::tiny-process-library::tiny-process-library)
else()
    target_link_libraries(rwrapper tiny-process-library::tiny-process-library)
endif()
