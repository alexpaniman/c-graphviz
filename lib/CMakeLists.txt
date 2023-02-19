cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------- TESTING ---------------------------------

# Make test-framework available for linking
add_subdirectory(test-framework)

# Enable ctest in current project
enable_testing()

# Macro to add test and remeber it's targets
# It's not very useful now, but it will be, when we add more tests
macro(add_unit_test target lib-under-test target-cpp)
  # Create test executable
  add_executable(${target} ${target-cpp})

  # Link library that we're testing to that executable
  target_link_libraries(${target} PUBLIC ${lib-under-test} test-framework)

  # Keep track of test targets
  set(UNIT_TEST_TARGETS ${UNIT_TEST_TARGETS} ${target} PARENT_SCOPE)

  # Add this unit test to other ones
  add_test(target ${CMAKE_CURRENT_BINARY_DIR}/${target})
endmacro(add_unit_test)

# Add target that depends on tests' tagets added with add_unit_test
add_custom_target(all_tests ALL DEPENDS ${UNIT_TEST_TARGETS})

# ------------------------------ DEPENDENCIES -------------------------------

# Simple stack implementation
add_subdirectory(simple-stack)

# Library for keeping track of errors
add_subdirectory(trace)

# Header only library for coloring terminal text
add_subdirectory(ansi-colors)

# Fully featured doubly linked list in array
add_subdirectory(linked-list)

# Library for text processing derived from onegin
add_subdirectory(textlib)

# Fast hash table implementation, that depends on list
add_subdirectory(hash-table)

# Safe alternatives to alloc function family, that uses trace
add_subdirectory(safe-alloc)

# Library for graph visualization
add_subdirectory(graphviz)

# Macro helper
add_subdirectory(macro-utils)
