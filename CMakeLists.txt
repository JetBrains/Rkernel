cmake_minimum_required(VERSION 3.6)
project(rwrapper)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED)

if (NOT DEFINED GRPC_DIST)
  set(GRPC_DIST ${CMAKE_HOME_DIRECTORY}/grpc)
endif()

if(NOT DEFINED R_HOME AND DEFINED ENV{R_HOME})
  set(R_HOME $ENV{R_HOME})
endif()
if (NOT DEFINED R_HOME)
    execute_process (
            COMMAND R RHOME
            OUTPUT_VARIABLE R_HOME
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()
message("R_HOME is set to '${R_HOME}'")

if(NOT DEFINED R_INCLUDE AND DEFINED ENV{R_INCLUDE})
  set(R_INCLUDE $ENV{R_INCLUDE})
endif()
if (NOT DEFINED R_INCLUDE)
    if(WIN32)
        string(REPLACE "\"" "" R_HOME ${R_HOME})
        message("${R_HOME}/bin/x64/R --slave -e \"cat\(R.home\('include'\)\)\"")
        execute_process (
                  COMMAND ${R_HOME}/bin/x64/R --slave -e "cat\(R.home\('include'\)\)"
                  OUTPUT_VARIABLE R_INCLUDE
              )
    else()
      execute_process (
          COMMAND ${R_HOME}/bin/R --slave -e cat\(R.home\('include'\)\)
          OUTPUT_VARIABLE R_INCLUDE
      )
    endif()
endif()
message("R_INCLUDE is set to ${R_INCLUDE}")

if(WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_WIN32_WINNT=0x600)
    if (DEFINED MSVC)
      set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    endif()
endif()

set(_PROTOBUF_PROTOC ${GRPC_DIST}/tools/protobuf/protoc${CMAKE_EXECUTABLE_SUFFIX})
set(_GRPC_CPP_PLUGIN_EXECUTABLE ${GRPC_DIST}/tools/grpc/grpc_cpp_plugin${CMAKE_EXECUTABLE_SUFFIX})
set(service_proto "${CMAKE_HOME_DIRECTORY}/Rkernel-proto/service.proto")
get_filename_component(service_proto_path "${service_proto}" DIRECTORY)
set(service_proto_srcs "${CMAKE_HOME_DIRECTORY}/gens/protos/service.pb.cc")
set(service_proto_hdrs "${CMAKE_HOME_DIRECTORY}/gens/protos/service.pb.h")
set(service_grpc_srcs "${CMAKE_HOME_DIRECTORY}/gens/protos/service.grpc.pb.cc")
set(service_grpc_hdrs "${CMAKE_HOME_DIRECTORY}/gens/protos/service.grpc.pb.h")
add_custom_command(
        OUTPUT "${service_proto_srcs}" "${service_proto_hdrs}" "${service_grpc_srcs}" "${service_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_HOME_DIRECTORY}/gens/protos"
            --cpp_out "${CMAKE_HOME_DIRECTORY}/gens/protos"
            -I "${service_proto_path}"
            -I "${GRPC_DIST}/include"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${service_proto}"
        DEPENDS "${service_proto}")
include_directories("${CMAKE_HOME_DIRECTORY}/gens")
include_directories("${R_INCLUDE}")
include_directories("${CMAKE_HOME_DIRECTORY}/Rcpp/inst/include")
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

if (UNIX)
    add_library(grpc++_unsecure STATIC IMPORTED)
    set_target_properties(grpc++_unsecure PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc++_unsecure${CMAKE_STATIC_LIBRARY_SUFFIX}")
    add_library(gpr STATIC IMPORTED)
    set_target_properties(gpr PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgpr${CMAKE_STATIC_LIBRARY_SUFFIX}")
    add_library(grpc_unsecure STATIC IMPORTED)
    set_target_properties(grpc_unsecure PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc_unsecure${CMAKE_STATIC_LIBRARY_SUFFIX}")
    add_library(grpc++_reflection STATIC IMPORTED)
    set_target_properties(grpc++_reflection PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc++_reflection${CMAKE_STATIC_LIBRARY_SUFFIX}")
    add_library(protobuf STATIC IMPORTED)
    set_target_properties(protobuf PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libprotobuf${CMAKE_STATIC_LIBRARY_SUFFIX}")
    add_library(libz STATIC IMPORTED)
    set_target_properties(libz PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libz${CMAKE_STATIC_LIBRARY_SUFFIX}")
    add_library(libssl STATIC IMPORTED)
    set_target_properties(libssl PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libssl${CMAKE_STATIC_LIBRARY_SUFFIX}")
    add_library(libcares STATIC IMPORTED)
    set_target_properties(libcares PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libcares${CMAKE_STATIC_LIBRARY_SUFFIX}")
    add_library(libaddress_sorting STATIC IMPORTED)
    set_target_properties(libaddress_sorting PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libaddress_sorting${CMAKE_STATIC_LIBRARY_SUFFIX}")
elseif(WIN32)
    link_directories("${GRPC_DIST}/lib")
endif()


add_executable(
        rwrapper
        ${RWRAPPER_MAIN}
        src/IO.cpp
        src/RPIServiceImpl.cpp
        src/RcppExports.cpp
        src/graphics/Evaluator.cpp
        src/graphics/Interface.cpp
        src/graphics/MasterDevice.cpp
        src/graphics/ScopeProtector.cpp
        src/graphics/SlaveDevice.cpp
        src/graphics/devices/RLazyGraphicsDevice.cpp
        src/graphics/devices/REagerGraphicsDevice.cpp
        src/graphics/devices/actions/RCircleAction.cpp
        src/graphics/devices/actions/RClipAction.cpp
        src/graphics/devices/actions/RLineAction.cpp
        src/graphics/devices/actions/RModeAction.cpp
        src/graphics/devices/actions/RNewPageAction.cpp
        src/graphics/devices/actions/RPolygonAction.cpp
        src/graphics/devices/actions/RPolylineAction.cpp
        src/graphics/devices/actions/RRectAction.cpp
        src/graphics/devices/actions/RPathAction.cpp
        src/graphics/devices/actions/RRasterAction.cpp
        src/graphics/devices/actions/RTextAsciiAction.cpp
        src/graphics/devices/actions/RTextUtf8Action.cpp
        src/graphics/devices/actions/util/RescaleUtil.cpp
        src/base64/base64.cpp
        src/base64/base64r.cpp
        src/RPIServiceMethods.cpp
        src/RRefs.cpp
        src/Repl.cpp
        src/RLoader.cpp
        src/DataFrame.cpp
        src/RDebugger.cpp
        src/Options.cpp
        ${service_proto_srcs}
        ${service_grpc_srcs}
)

add_library(rcpp_static ${CMAKE_HOME_DIRECTORY}/Rcpp/src/api.cpp
                        ${CMAKE_HOME_DIRECTORY}/Rcpp/src/attributes.cpp
                        ${CMAKE_HOME_DIRECTORY}/Rcpp/src/barrier.cpp
                        ${CMAKE_HOME_DIRECTORY}/Rcpp/src/date.cpp
                        ${CMAKE_HOME_DIRECTORY}/Rcpp/src/module.cpp
                        ${CMAKE_HOME_DIRECTORY}/Rcpp/src/rcpp_init.cpp)

if (UNIX)
  target_link_libraries(rwrapper grpc++_unsecure grpc_unsecure protobuf libz libssl libcares libaddress_sorting  gpr)
  if (NOT APPLE)
      target_link_libraries(rwrapper pthread)
  endif()
endif()
if (WIN32)
  target_link_libraries(rwrapper grpc++_unsecure grpc_unsecure libprotobuf zlib ssleay32 cares address_sorting  gpr)
  target_link_libraries(rwrapper WS2_32)
  target_link_libraries(rwrapper Rgraphapp)
endif()

target_link_libraries(rwrapper rcpp_static)
target_link_libraries(rwrapper R)
