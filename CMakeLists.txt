cmake_minimum_required(VERSION 3.6)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")
project(rwrapper)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED)

add_compile_definitions(__APPLE_SILICON__)

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
    if (DEFINED MSVC)
      set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /stack:0x1000000")
endif()

if(APPLE)
    add_definitions(-D_MACOS)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DRWRAPPER_DEBUG")

if (DEFINED CRASHPAD_DIR)
    message("Crashpad is enabled")
    add_definitions( -DCRASHPAD_ENABLED )

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
    file(GLOB_RECURSE CRASHPAD_LIBUTIL   "${CRASHPAD_LIBRARY_DIR}/*util${CMAKE_STATIC_LIBRARY_SUFFIX}")
    file(GLOB_RECURSE CRASHPAD_LIBBASE   "${CRASHPAD_LIBRARY_DIR}/*base${CMAKE_STATIC_LIBRARY_SUFFIX}")
    file(GLOB_RECURSE CRASHPAD_LIBCOMPAT   "${CRASHPAD_LIBRARY_DIR}/*compat${CMAKE_STATIC_LIBRARY_SUFFIX}")
    list (APPEND CRASHPAD_LIBRARIES ${CRASHPAD_LIBCLIENT} ${CRASHPAD_LIBUTIL} ${CRASHPAD_LIBBASE} ${CRASHPAD_LIBCOMPAT})
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

if (UNIX)
    if (APPLE AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
        add_library(libabsl_bad_any_cast_impl STATIC IMPORTED)
        set_target_properties(libabsl_bad_any_cast_impl PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_bad_any_cast_impl${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_bad_optional_access STATIC IMPORTED)
        set_target_properties(libabsl_bad_optional_access PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_bad_optional_access${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_bad_variant_access STATIC IMPORTED)
        set_target_properties(libabsl_bad_variant_access PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_bad_variant_access${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_base STATIC IMPORTED)
        set_target_properties(libabsl_base PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_base${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_city STATIC IMPORTED)
        set_target_properties(libabsl_city PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_city${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_civil_time STATIC IMPORTED)
        set_target_properties(libabsl_civil_time PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_civil_time${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_cord STATIC IMPORTED)
        set_target_properties(libabsl_cord PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_cord${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_debugging_internal STATIC IMPORTED)
        set_target_properties(libabsl_debugging_internal PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_debugging_internal${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_demangle_internal STATIC IMPORTED)
        set_target_properties(libabsl_demangle_internal PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_demangle_internal${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_examine_stack STATIC IMPORTED)
        set_target_properties(libabsl_examine_stack PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_examine_stack${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_exponential_biased STATIC IMPORTED)
        set_target_properties(libabsl_exponential_biased PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_exponential_biased${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_failure_signal_handler STATIC IMPORTED)
        set_target_properties(libabsl_failure_signal_handler PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_failure_signal_handler${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags STATIC IMPORTED)
        set_target_properties(libabsl_flags PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_commandlineflag STATIC IMPORTED)
        set_target_properties(libabsl_flags_commandlineflag PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_commandlineflag${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_commandlineflag_internal STATIC IMPORTED)
        set_target_properties(libabsl_flags_commandlineflag_internal PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_commandlineflag_internal${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_config STATIC IMPORTED)
        set_target_properties(libabsl_flags_config PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_config${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_internal STATIC IMPORTED)
        set_target_properties(libabsl_flags_internal PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_internal${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_marshalling STATIC IMPORTED)
        set_target_properties(libabsl_flags_marshalling PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_marshalling${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_parse STATIC IMPORTED)
        set_target_properties(libabsl_flags_parse PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_parse${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_private_handle_accessor STATIC IMPORTED)
        set_target_properties(libabsl_flags_private_handle_accessor PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_private_handle_accessor${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_program_name STATIC IMPORTED)
        set_target_properties(libabsl_flags_program_name PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_program_name${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_reflection STATIC IMPORTED)
        set_target_properties(libabsl_flags_reflection PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_reflection${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_usage STATIC IMPORTED)
        set_target_properties(libabsl_flags_usage PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_usage${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_flags_usage_internal STATIC IMPORTED)
        set_target_properties(libabsl_flags_usage_internal PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_flags_usage_internal${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_graphcycles_internal STATIC IMPORTED)
        set_target_properties(libabsl_graphcycles_internal PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_graphcycles_internal${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_hash STATIC IMPORTED)
        set_target_properties(libabsl_hash PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_hash${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_hashtablez_sampler STATIC IMPORTED)
        set_target_properties(libabsl_hashtablez_sampler PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_hashtablez_sampler${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_int128 STATIC IMPORTED)
        set_target_properties(libabsl_int128 PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_int128${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_leak_check STATIC IMPORTED)
        set_target_properties(libabsl_leak_check PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_leak_check${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_leak_check_disable STATIC IMPORTED)
        set_target_properties(libabsl_leak_check_disable PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_leak_check_disable${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_log_severity STATIC IMPORTED)
        set_target_properties(libabsl_log_severity PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_log_severity${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_malloc_internal STATIC IMPORTED)
        set_target_properties(libabsl_malloc_internal PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_malloc_internal${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_periodic_sampler STATIC IMPORTED)
        set_target_properties(libabsl_periodic_sampler PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_periodic_sampler${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_distributions STATIC IMPORTED)
        set_target_properties(libabsl_random_distributions PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_distributions${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_internal_distribution_test_util STATIC IMPORTED)
        set_target_properties(libabsl_random_internal_distribution_test_util PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_internal_distribution_test_util${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_internal_platform STATIC IMPORTED)
        set_target_properties(libabsl_random_internal_platform PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_internal_platform${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_internal_pool_urbg STATIC IMPORTED)
        set_target_properties(libabsl_random_internal_pool_urbg PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_internal_pool_urbg${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_internal_randen STATIC IMPORTED)
        set_target_properties(libabsl_random_internal_randen PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_internal_randen${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_internal_randen_hwaes STATIC IMPORTED)
        set_target_properties(libabsl_random_internal_randen_hwaes PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_internal_randen_hwaes${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_internal_randen_hwaes_impl STATIC IMPORTED)
        set_target_properties(libabsl_random_internal_randen_hwaes_impl PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_internal_randen_hwaes_impl${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_internal_randen_slow STATIC IMPORTED)
        set_target_properties(libabsl_random_internal_randen_slow PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_internal_randen_slow${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_internal_seed_material STATIC IMPORTED)
        set_target_properties(libabsl_random_internal_seed_material PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_internal_seed_material${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_seed_gen_exception STATIC IMPORTED)
        set_target_properties(libabsl_random_seed_gen_exception PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_seed_gen_exception${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_random_seed_sequences STATIC IMPORTED)
        set_target_properties(libabsl_random_seed_sequences PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_random_seed_sequences${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_raw_hash_set STATIC IMPORTED)
        set_target_properties(libabsl_raw_hash_set PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_raw_hash_set${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_raw_logging_internal STATIC IMPORTED)
        set_target_properties(libabsl_raw_logging_internal PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_raw_logging_internal${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_scoped_set_env STATIC IMPORTED)
        set_target_properties(libabsl_scoped_set_env PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_scoped_set_env${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_spinlock_wait STATIC IMPORTED)
        set_target_properties(libabsl_spinlock_wait PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_spinlock_wait${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_stacktrace STATIC IMPORTED)
        set_target_properties(libabsl_stacktrace PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_stacktrace${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_status STATIC IMPORTED)
        set_target_properties(libabsl_status PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_status${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_statusor STATIC IMPORTED)
        set_target_properties(libabsl_statusor PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_statusor${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_str_format_internal STATIC IMPORTED)
        set_target_properties(libabsl_str_format_internal PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_str_format_internal${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_strerror STATIC IMPORTED)
        set_target_properties(libabsl_strerror PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_strerror${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_strings STATIC IMPORTED)
        set_target_properties(libabsl_strings PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_strings${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_strings_internal STATIC IMPORTED)
        set_target_properties(libabsl_strings_internal PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_strings_internal${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_symbolize STATIC IMPORTED)
        set_target_properties(libabsl_symbolize PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_symbolize${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_synchronization STATIC IMPORTED)
        set_target_properties(libabsl_synchronization PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_synchronization${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_throw_delegate STATIC IMPORTED)
        set_target_properties(libabsl_throw_delegate PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_throw_delegate${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_time STATIC IMPORTED)
        set_target_properties(libabsl_time PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_time${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_time_zone STATIC IMPORTED)
        set_target_properties(libabsl_time_zone PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_time_zone${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libabsl_wyhash STATIC IMPORTED)
        set_target_properties(libabsl_wyhash PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libabsl_wyhash${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libaddress_sorting STATIC IMPORTED)
        set_target_properties(libaddress_sorting PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libaddress_sorting${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libcares STATIC IMPORTED)
        set_target_properties(libcares PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libcares${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libcrypto STATIC IMPORTED)
        set_target_properties(libcrypto PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgpr STATIC IMPORTED)
        set_target_properties(libgpr PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgpr${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgrpc++ STATIC IMPORTED)
        set_target_properties(libgrpc++ PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc++${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgrpc++_alts STATIC IMPORTED)
        set_target_properties(libgrpc++_alts PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc++_alts${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgrpc++_error_details STATIC IMPORTED)
        set_target_properties(libgrpc++_error_details PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc++_error_details${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgrpc++_reflection STATIC IMPORTED)
        set_target_properties(libgrpc++_reflection PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc++_reflection${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgrpc++_unsecure STATIC IMPORTED)
        set_target_properties(libgrpc++_unsecure PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc++_unsecure${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgrpc STATIC IMPORTED)
        set_target_properties(libgrpc PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgrpc_plugin_support STATIC IMPORTED)
        set_target_properties(libgrpc_plugin_support PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc_plugin_support${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgrpc_unsecure STATIC IMPORTED)
        set_target_properties(libgrpc_unsecure PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc_unsecure${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgrpc_upbdefs STATIC IMPORTED)
        set_target_properties(libgrpc_upbdefs PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpc_upbdefs${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libgrpcpp_channelz STATIC IMPORTED)
        set_target_properties(libgrpcpp_channelz PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libgrpcpp_channelz${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libprotobuf-lite STATIC IMPORTED)
        set_target_properties(libprotobuf-lite PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libprotobuf-lite${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libprotobuf STATIC IMPORTED)
        set_target_properties(libprotobuf PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libprotobuf${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libprotoc STATIC IMPORTED)
        set_target_properties(libprotoc PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libprotoc${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libre2 STATIC IMPORTED)
        set_target_properties(libre2 PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libre2${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libssl STATIC IMPORTED)
        set_target_properties(libssl PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libssl${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libtiny-process-library STATIC IMPORTED)
        set_target_properties(libtiny-process-library PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libtiny-process-library${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libupb STATIC IMPORTED)
        set_target_properties(libupb PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libupb${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libupb_fastdecode STATIC IMPORTED)
        set_target_properties(libupb_fastdecode PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libupb_fastdecode${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libupb_handlers STATIC IMPORTED)
        set_target_properties(libupb_handlers PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libupb_handlers${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libupb_json STATIC IMPORTED)
        set_target_properties(libupb_json PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libupb_json${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libupb_pb STATIC IMPORTED)
        set_target_properties(libupb_pb PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libupb_pb${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libupb_reflection STATIC IMPORTED)
        set_target_properties(libupb_reflection PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libupb_reflection${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libupb_textformat STATIC IMPORTED)
        set_target_properties(libupb_textformat PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libupb_textformat${CMAKE_STATIC_LIBRARY_SUFFIX}")
        add_library(libz STATIC IMPORTED)
        set_target_properties(libz PROPERTIES IMPORTED_LOCATION "${GRPC_DIST}/lib/libz${CMAKE_STATIC_LIBRARY_SUFFIX}")
    else()
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
    endif()
endif()

link_directories("${GRPC_DIST}/lib")

if (UNIX)
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

if (DEFINED CRASHPAD_DIR)
    target_link_libraries(rwrapper ${CRASHPAD_LIBRARIES})
endif()

if (UNIX)
  if (APPLE AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
      target_link_libraries(rwrapper "-framework Foundation" grpc++_unsecure grpc_unsecure protobuf libz libssl libaddress_sorting  gpr libabsl_bad_any_cast_impl libabsl_bad_optional_access libabsl_bad_variant_access libabsl_base libabsl_city libabsl_civil_time libabsl_cord libabsl_debugging_internal libabsl_demangle_internal libabsl_examine_stack libabsl_exponential_biased libabsl_failure_signal_handler libabsl_flags libabsl_flags_commandlineflag libabsl_flags_commandlineflag_internal libabsl_flags_config libabsl_flags_internal libabsl_flags_marshalling libabsl_flags_parse libabsl_flags_private_handle_accessor libabsl_flags_program_name libabsl_flags_reflection libabsl_flags_usage libabsl_flags_usage_internal libabsl_graphcycles_internal libabsl_hash libabsl_hashtablez_sampler libabsl_int128 libabsl_leak_check libabsl_leak_check_disable libabsl_log_severity libabsl_malloc_internal libabsl_periodic_sampler libabsl_random_distributions libabsl_random_internal_distribution_test_util libabsl_random_internal_platform libabsl_random_internal_pool_urbg libabsl_random_internal_randen libabsl_random_internal_randen_hwaes libabsl_random_internal_randen_hwaes_impl libabsl_random_internal_randen_slow libabsl_random_internal_seed_material libabsl_random_seed_gen_exception libabsl_random_seed_sequences libabsl_raw_hash_set libabsl_raw_logging_internal libabsl_scoped_set_env libabsl_spinlock_wait libabsl_stacktrace libabsl_status libabsl_statusor libabsl_str_format_internal libabsl_strerror libabsl_strings libabsl_strings_internal libabsl_symbolize libabsl_synchronization libabsl_throw_delegate libabsl_time libabsl_time_zone libabsl_wyhash libaddress_sorting libcares libcrypto libgpr libgrpc++ libgrpc++_alts libgrpc++_error_details libgrpc++_reflection libgrpc++_unsecure libgrpc libgrpc_plugin_support libgrpc_unsecure libgrpc_upbdefs libgrpcpp_channelz libprotobuf-lite libprotobuf libprotoc libre2 libssl libtiny-process-library libupb libupb_fastdecode libupb_handlers libupb_json libupb_pb libupb_reflection libupb_textformat libz -lresolv)
  else()
      target_link_libraries(rwrapper grpc++_unsecure grpc_unsecure protobuf libz libssl libcares libaddress_sorting  gpr)
      if (NOT APPLE)
          target_link_libraries(rwrapper pthread dl)
      endif()
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif()
if (WIN32)
  target_link_libraries(rwrapper grpc++_unsecure grpc_unsecure libprotobuf zlib ssleay32 cares address_sorting  gpr)
  target_link_libraries(rwrapper WS2_32)
  target_link_libraries(rwrapper Rgraphapp)
endif()

target_link_libraries(rwrapper R)
target_link_libraries(rwrapper tiny-process-library)
