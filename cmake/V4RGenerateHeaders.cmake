# This script generates a file "v4r/config.h" with V4R library configuration
# It contains defines for enabled third-party libraries and V4R (sub)modules

set(V4R_HAVE_CHECKS "")
set(V4R_MODULE_DEFINITIONS "")
set(V4R_3P_DEFINITIONS "")

set(_module_list ${V4R_MODULES})
v4r_list_sort(_module_list)
foreach(_module ${_module_list})
  string(TOUPPER "${_module}" _MODULE)
  set(_state 0)
  if(HAVE_${_module})
    set(_state 1)
    set(V4R_HAVE_CHECKS "${V4R_HAVE_CHECKS}\n     ||  stringsEqual(\"${_module}\", what)")
  endif()
  set(V4R_MODULE_DEFINITIONS "${V4R_MODULE_DEFINITIONS}#define HAVE_${_MODULE} ${_state}\n")
  foreach(_submodule ${V4R_MODULE_${_module}_SUBMODULES})
    set(_state 0)
    string(TOUPPER "${_submodule}" _SUBMODULE)
    if(HAVE_${_submodule})
      set(_state 1)
      set(V4R_HAVE_CHECKS "${V4R_HAVE_CHECKS}\n     ||  stringsEqual(\"${_submodule}\", what)")
    endif()
    set(V4R_MODULE_DEFINITIONS "${V4R_MODULE_DEFINITIONS}#define HAVE_${_SUBMODULE} ${_state}\n")
  endforeach()
endforeach()

set(_3p_list ${V4R_3P_ALL})
v4r_list_sort(_3p_list)
foreach(_3P ${_3p_list})
  string(TOLOWER "${_3P}" _3p)
  set(_state 0)
  if(HAVE_${_3P})
    set(_state 1)
    set(V4R_HAVE_CHECKS "${V4R_HAVE_CHECKS}\n     ||  stringsEqual(\"${_3p}\", what)")
  endif()
  set(V4R_3P_DEFINITIONS "${V4R_3P_DEFINITIONS}#define HAVE_${_3P} ${_state}\n")
endforeach()

# Platform-specific config file with list of enabled dependencies and modules
configure_file("${V4R_SOURCE_DIR}/cmake/templates/config.h.in" "${V4R_GENERATED_HEADERS_DIR}/v4r/config.h")
install(DIRECTORY "${V4R_GENERATED_HEADERS_DIR}/v4r" DESTINATION ${V4R_INCLUDE_INSTALL_PATH} COMPONENT dev)
