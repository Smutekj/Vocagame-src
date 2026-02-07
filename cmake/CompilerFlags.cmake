function(set_target_compiler_flags target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE $<$<CONFIG:Release>:/O2>)
    target_compile_options(${target_name} PRIVATE /MP)
  else()
    target_compile_options(${target_name} PRIVATE $<$<CONFIG:Release>:-O3>)
    # target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug>:-Q>)
  endif()
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON) ## to see the compiler flags
  set(CMAKE_EXPORT_LINK_COMMANDS ON) ## to see the compiler flags
  set(CMAKE_CXX_COMPILER_FLAGS "${CMAKE_CXX_COMPILER_FLAGS} EXIT_RUNTIME=1")
  set(CMAKE_C_COMPILER_FLAGS "${CMAKE_C_COMPILER_FLAGS} EXIT_RUNTIME=1")
endfunction()

