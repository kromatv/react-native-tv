# SDL 3 from webosbrew's fork, linked statically: the window, the OpenGL ES
# context, remote keys, the pointer and the on-screen keyboard. The TV's own
# libSDL2 is a different library on each webOS release
# (docs/reference/webos-window-input.md, section 6). With WEBOS off the fork is
# stock SDL, which the simulator runs on X11.

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
foreach(_subsystem AUDIO GPU RENDER CAMERA JOYSTICK HAPTIC HIDAPI POWER SENSOR DIALOG)
  set(SDL_${_subsystem} OFF CACHE BOOL "" FORCE)
endforeach()
set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES ON CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
set(SDL_KMSDRM OFF CACHE BOOL "" FORCE)
set(SDL_OFFSCREEN OFF CACHE BOOL "" FORCE)
set(SDL_DBUS OFF CACHE BOOL "" FORCE)
set(SDL_IBUS OFF CACHE BOOL "" FORCE)
set(SDL_LIBUDEV OFF CACHE BOOL "" FORCE)
if(RN_WEBOS_SIMULATOR)
  set(SDL_X11 ON CACHE BOOL "" FORCE)
  set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)
else()
  include("${CMAKE_CURRENT_LIST_DIR}/WaylandScanner.cmake")
  set(WEBOS ON CACHE BOOL "" FORCE)
  set(SDL_X11 OFF CACHE BOOL "" FORCE)
  set(SDL_WAYLAND ON CACHE BOOL "" FORCE)
  set(SDL_WAYLAND_LIBDECOR OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory("${sdl_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/sdl" EXCLUDE_FROM_ALL)
target_compile_options(SDL3-static PRIVATE -Os -ffunction-sections -fdata-sections)

if(NOT RN_WEBOS_SIMULATOR)
  # SDL calls itself through a table of its whole API, so that a user can put
  # another SDL in its place at run time, and the table keeps every function
  # in the executable. A TV app's SDL is this one. SDL reserves the switch to
  # a hand edit of SDL_dynapi.h; defining that header's guard is the edit.
  target_compile_definitions(SDL3-static PRIVATE SDL_dynapi_h_ SDL_DYNAMIC_API=0)
  set_source_files_properties("${sdl_SOURCE_DIR}/src/hidapi/SDL_hidapi.c"
    TARGET_DIRECTORY SDL3-static
    PROPERTIES COMPILE_OPTIONS "-include;${CMAKE_CURRENT_LIST_DIR}/../sdl/hidapi-disabled.h")
endif()
