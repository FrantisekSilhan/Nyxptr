cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(Nyxptr VERSION 0.0.1 LANGUAGES C CXX)

if (WIN32)
  list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libtorch")
endif()
find_package(Torch REQUIRED)
string(REPLACE "-std=c++17" "" TORCH_CXX_FLAGS "${TORCH_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

function(remove_std17_from_target target)
  if(TARGET ${target})
    get_target_property(options ${target} INTERFACE_COMPILE_OPTIONS)
    if(options)
      list(REMOVE_ITEM options "-std=c++17")
      set_target_properties(${target} PROPERTIES INTERFACE_COMPILE_OPTIONS "${options}")
    endif()
  endif()
endfunction()

remove_std17_from_target(torch)
remove_std17_from_target(torch_cpu)
remove_std17_from_target(torch_cuda)
remove_std17_from_target(torch_hip)
remove_std17_from_target(c10)
remove_std17_from_target(c10_hip)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES
  src/*.cpp
)

set(FATHOM_SOURCE "ext/Fathom/tbprobe.c")

add_executable(${PROJECT_NAME} ${SOURCES} ${FATHOM_SOURCE})

set_source_files_properties(${FATHOM_SOURCE} PROPERTIES LANGUAGE C)
set_source_files_properties(${FATHOM_SOURCE} PROPERTIES COMPILE_DEFINITIONS "TB_NO_THREADS;_CRT_SECURE_NO_WARNINGS")
if (MSVC)
  set_source_files_properties(${FATHOM_SOURCE} PROPERTIES COMPILE_FLAGS "/W0")
else()
  set_source_files_properties(${FATHOM_SOURCE} PROPERTIES COMPILE_FLAGS "-w")
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ext/Fathom
)

target_link_libraries(${PROJECT_NAME} ${TORCH_LIBRARIES})
get_target_property(TORCH_INCLUDE_DIRST torch INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${TORCH_INCLUDE_DIRS})

set_target_properties(${PROJECT_NAME} PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

if (MSVC)
  file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${TORCH_DLLS}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX /wd4267 /wd4702)
else()
  target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _GLIBCXX_USE_CXX11_ABI=1)
  set_target_properties(${PROJECT_NAME} PROPERTIES 
    INSTALL_RPATH "/opt/rocm/lib:/opt/libtorch/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
  )
endif()
