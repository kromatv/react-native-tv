# Stand-ins for the Tizen native APIs the host calls that Linux has no
# equivalent for: dlog, system_info, app_common, app, efl-extension and
# capi-media-player, which has no player here.

add_library(rn-tizen-simulator-shims STATIC src/app.c src/dlog.c src/efl_extension.c src/player.c src/system_info.c)
target_include_directories(rn-tizen-simulator-shims PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_options(rn-tizen-simulator-shims PRIVATE -Wall -Wextra -Werror)
target_link_libraries(rn-tizen-simulator-shims PUBLIC PkgConfig::RN_TIZEN_SIMULATOR_EFL)
