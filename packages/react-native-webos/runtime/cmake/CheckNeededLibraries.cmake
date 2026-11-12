# Fails when an ELF file needs a shared library outside a list.
#
#   cmake -DREADELF=<llvm-readelf> -DBINARY=<file> -DALLOWED=<soname,soname> -P CheckNeededLibraries.cmake

execute_process(
  COMMAND "${READELF}" --needed-libs "${BINARY}"
  OUTPUT_VARIABLE _needed COMMAND_ERROR_IS_FATAL ANY)
string(REPLACE "," ";" _allowed "${ALLOWED}")
string(REGEX MATCHALL "[^ \t\r\n]+\\.so[.0-9]*" _needed "${_needed}")
set(_unsafe "")
foreach(_soname IN LISTS _needed)
  if(NOT _soname IN_LIST _allowed)
    list(APPEND _unsafe "${_soname}")
  endif()
endforeach()
if(_unsafe)
  list(JOIN _unsafe ", " _unsafe)
  message(FATAL_ERROR
    "${BINARY} needs ${_unsafe}: not every webOS release has it under that name "
    "(docs/reference/webos-native-toolchain.md).")
endif()
