
function(global_add_define NAME VALUE)
  appendprop(DEFINES_KEYS ${NAME})
  appendprop(DEFINES_VALUES ${VALUE})
endfunction(global_add_define)

function(global_add_define_bool NAME VALUE)
  if ("${VALUE}" STREQUAL "0")
    set(VALUE "TIKI_OFF")
  elseif("${VALUE}" STREQUAL "1")
    set(VALUE "TIKI_ON")
  endif()
  
  global_add_define(${NAME} ${VALUE})
endfunction(global_add_define_bool)

function(module_add_define NAME VALUE)
  getprop(MODULE_NAME CURRENT_MODULE_NAME)
  
  set(KEYS_VAR ${MODULE_NAME}_DEFINES_KEYS)
  set(VALUES_VAR ${MODULE_NAME}_DEFINES_VALUES)
  
  appendprop(${KEYS_VAR} ${NAME})
  appendprop(${VALUES_VAR} ${VALUE})
endfunction(module_add_define)

function(module_add_define_bool NAME VALUE)
  if ("${VALUE}" STREQUAL "0")
    set(VALUE "TIKI_OFF")
  elseif("${VALUE}" STREQUAL "1")
    set(VALUE "TIKI_ON")
  endif()
  
  module_add_define(${NAME} ${VALUE})
endfunction(module_add_define_bool)

function(finalize_define_write FILE_NAME KEYS_VAR VALUES_VAR)
  getprop(DEFINES_KEYS ${KEYS_VAR})
  getprop(DEFINES_VALUES ${VALUES_VAR})

  set(I 0)
  list(LENGTH DEFINES_KEYS I_COUNT)
  
  while(I LESS I_COUNT)
    list(GET DEFINES_KEYS ${I} KEY)
    list(GET DEFINES_VALUES ${I} VALUE)
    
    file(APPEND ${FILE_NAME} "#define ${KEY} ${VALUE}\n")

    math(EXPR I "${I}+1")
  endwhile()
endfunction(finalize_define_write)

function(finalize_define MODULE_NAME DEP_MODULES)
  string(TOLOWER ${MODULE_NAME} LNAME)
  string(TOUPPER ${MODULE_NAME} UNAME)
  set(DEFINES_HPP ${CMAKE_CURRENT_BINARY_DIR}/define/define_${LNAME}.hpp)
  set(DEFINES_HPP_TMP ${DEFINES_HPP_TMP}.tmp)

  file(WRITE ${DEFINES_HPP_TMP} "#ifndef TIKI_${UNAME}_DEFINES_HPP\n")
  file(APPEND ${DEFINES_HPP_TMP} "#define TIKI_${UNAME}_DEFINES_HPP\n\n") 
  
  file(APPEND ${DEFINES_HPP_TMP} "#define TIKI_ON 2\n")
  file(APPEND ${DEFINES_HPP_TMP} "#define TIKI_OFF 1\n\n")
  
  finalize_define_write(${DEFINES_HPP_TMP} DEFINES_KEYS DEFINES_VALUES)

  finalize_define_write(${DEFINES_HPP_TMP} ${UNAME}_DEFINES_KEYS ${UNAME}_DEFINES_VALUES)

  foreach(MOD ${DEP_MODULES})
    finalize_define_write(${DEFINES_HPP_TMP} ${MOD}_DEFINES_KEYS ${MOD}_DEFINES_VALUES)
  endforeach(MOD)
  
  file(APPEND ${DEFINES_HPP_TMP} "\n#endif // TIKI_${UNAME}_DEFINES_HPP")
  
  file(READ ${DEFINES_HPP_TMP} DEFINES_CONTENT)

  if(EXISTS ${DEFINES_HPP})
	file(READ ${DEFINES_HPP} DEFINES_CONTENT_OLDFILE)
	
	if(NOT ${DEFINES_CONTENT} STREQUAL ${DEFINES_CONTENT_OLDFILE})
      message("Generate Defines: ${DEFINES_HPP}")
	  file(WRITE ${DEFINES_HPP} ${DEFINES_CONTENT})
	endif()
  else()
    message("Generate Defines: ${DEFINES_HPP}")
	file(WRITE ${DEFINES_HPP} ${DEFINES_CONTENT})
  endif()
  
  file(REMOVE ${DEFINES_HPP_TMP})  
endfunction(finalize_define)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

set(TIKI_BUILD_CONFIG 0)
set(TIKI_BUILD_DEBUG 0)
set(TIKI_BUILD_RELEASE 0)
set(TIKI_BUILD_MASTER 0)  

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(TIKI_BUILD_DEBUG 1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(TIKI_BUILD_RELEASE 1)
elseif(CMAKE_BUILD_TYPE STREQUAL "Master")
  set(TIKI_BUILD_MASTER 1)
else()
  set(TIKI_BUILD_CONFIG 1)  
endif()

global_add_define_bool(TIKI_BUILD_CONFIG ${TIKI_BUILD_CONFIG})
global_add_define_bool(TIKI_BUILD_DEBUG ${TIKI_BUILD_DEBUG})
global_add_define_bool(TIKI_BUILD_RELEASE ${TIKI_BUILD_RELEASE})
global_add_define_bool(TIKI_BUILD_MASTER ${TIKI_BUILD_MASTER})

if(CMAKE_CL_64)
  set(TIKI_BUILD_32BIT 0)
  set(TIKI_BUILD_64BIT 1)
else()
  set(TIKI_BUILD_32BIT 1)
  set(TIKI_BUILD_64BIT 0)
endif()

global_add_define_bool(TIKI_BUILD_32BIT ${TIKI_BUILD_32BIT})
global_add_define_bool(TIKI_BUILD_64BIT ${TIKI_BUILD_64BIT})

if(TIKI_PLATFORM STREQUAL "MacOSX")
  set(TIKI_PLATFORM_WIN 0)
  set(TIKI_PLATFORM_LINUX 0)
  set(TIKI_PLATFORM_APPLE 1)
elseif(TIKI_PLATFORM STREQUAL "Linux")
  set(TIKI_PLATFORM_WIN 0)
  set(TIKI_PLATFORM_LINUX 1)
  set(TIKI_PLATFORM_APPLE 0)
elseif(TIKI_PLATFORM STREQUAL "Windows")
  set(TIKI_PLATFORM_WIN 1)
  set(TIKI_PLATFORM_LINUX 0)
  set(TIKI_PLATFORM_APPLE 0)
else()
  message(ERROR "Platform not supported")
endif()

global_add_define_bool(TIKI_PLATFORM_WIN ${TIKI_PLATFORM_WIN})
global_add_define_bool(TIKI_PLATFORM_LINUX ${TIKI_PLATFORM_LINUX})
global_add_define_bool(TIKI_PLATFORM_APPLE ${TIKI_PLATFORM_APPLE})
