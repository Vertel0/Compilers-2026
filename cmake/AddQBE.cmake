macro(qbe_add_tool name)
  cmake_parse_arguments(ARG "PARTIAL_SOURCES_INTENDED" "" "" ${ARGN})
  set(sources ${ARG_UNPARSED_ARGUMENTS})

  add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/qbe_${name}_restricted.o
      COMMAND ${CMAKE_LINKER} -r -o ${CMAKE_CURRENT_BINARY_DIR}/qbe_${name}_restricted.o --whole-archive ${LIBQBE_PATH}
      COMMAND 
        if [ -s ${CMAKE_CURRENT_SOURCE_DIR}/hidden_list.txt ]; then
          objcopy ${CMAKE_CURRENT_BINARY_DIR}/qbe_${name}_restricted.o --localize-symbols=${CMAKE_CURRENT_SOURCE_DIR}/hidden_list.txt\;
        fi;
      DEPENDS ${LIBQBE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/hidden_list.txt
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Restricting symbols in libqbe[${name}]..."
  )

  add_custom_target(generate_qbe_${name}_obj DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/qbe_${name}_restricted.o")

  add_library(qbe_${name}_restricted STATIC IMPORTED)
  set_target_properties(qbe_${name}_restricted PROPERTIES
      IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/qbe_${name}_restricted.o"
  )
  add_dependencies(qbe_${name}_restricted generate_qbe_${name}_obj)

  add_executable(${name} ${sources})
  target_link_libraries(${name} PRIVATE qbe_${name}_restricted)

endmacro(qbe_add_tool)

macro(qbe_add_test name)
  set(PRIVATE_TESTS_PATH "${CMAKE_SOURCE_DIR}/test/private/${name}")

  add_test(
      NAME "${name}-public-suite"
      COMMAND python ${CMAKE_SOURCE_DIR}/utils/tester.py
          --checker ${name}
          --bin-path $<TARGET_FILE:${name}>
          --tests-dir ${CMAKE_CURRENT_SOURCE_DIR}
  )

  if (EXISTS ${PRIVATE_TESTS_PATH} AND "$ENV{GITHUB_ACTIONS}" STREQUAL "true")
      add_test(
          NAME "${name}-private-suite"
          COMMAND python ${CMAKE_SOURCE_DIR}/utils/tester.py
              --checker ${name}
              --bin-path $<TARGET_FILE:${name}>
              --tests-dir ${PRIVATE_TESTS_PATH}
      )
  endif()

endmacro(qbe_add_test name)


