cmake_minimum_required(VERSION 3.21)
project(PigeonNotes LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Slint 1.19 REQUIRED)
add_executable(PigeonNotes src/main.cpp)
target_link_libraries(PigeonNotes PRIVATE Slint::Slint)
slint_target_sources(PigeonNotes ui/main.slint EDITION 1.19)
if(WIN32)
  add_custom_command(TARGET PigeonNotes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:PigeonNotes> $<TARGET_RUNTIME_DLLS:PigeonNotes>
    COMMAND_EXPAND_LISTS)
endif()
