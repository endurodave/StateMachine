set(CPP_STATEMACHINE_LIBRARY_NAME Cpp_StateMachine)

set(C_STATEMACHINE_CSRC
    ${CMAKE_CURRENT_SOURCE_DIR}/Allocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Fault.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StateMachine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xallocator.cpp)

add_library(${CPP_STATEMACHINE_LIBRARY_NAME} ${C_STATEMACHINE_CSRC})

target_compile_definitions(
  ${CPP_STATEMACHINE_LIBRARY_NAME}
  PRIVATE STATIC_POOLS
          EXTERNAL_EVENT_NO_HEAP_DATA=1)
target_compile_options(
  ${CPP_STATEMACHINE_LIBRARY_NAME}
  PRIVATE -Wno-unused-parameter
          -Wno-unused-variable
  PUBLIC -Wno-unused-but-set-variable)
target_include_directories(${CPP_STATEMACHINE_LIBRARY_NAME}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})