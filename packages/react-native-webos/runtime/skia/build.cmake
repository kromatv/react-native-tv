# Runs Ninja on Skia's build directory, one build of it at a time: every
# runtime build with the same Skia arguments shares that directory, and two
# Ninja processes in one directory corrupt it.
#
#   cmake -DOUT=<dir> -DNINJA=<ninja> "-DTARGETS=skia skparagraph" -P build.cmake

separate_arguments(_targets UNIX_COMMAND "${TARGETS}")
file(LOCK "${OUT}/.rn-webos-build.lock" TIMEOUT 7200)
execute_process(COMMAND "${NINJA}" -C "${OUT}" ${_targets} RESULT_VARIABLE _result)
file(LOCK "${OUT}/.rn-webos-build.lock" RELEASE)
if(NOT _result EQUAL 0)
  message(FATAL_ERROR "Skia failed to build in ${OUT}")
endif()
