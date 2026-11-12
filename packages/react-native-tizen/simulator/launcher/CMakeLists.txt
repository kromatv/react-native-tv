# The simulator's stand-in for the TV's .NET shell (shell/): starts the host in
# an Elementary main loop. Lines written to the optional control FIFO drive the
# app's lifecycle: `pause`, `resume`, `open-url <url>` and `exit`.

add_executable(rn-tizen-simulator main.c)
target_link_libraries(rn-tizen-simulator PRIVATE react-native-tizen PkgConfig::RN_TIZEN_SIMULATOR_EFL)
target_compile_options(rn-tizen-simulator PRIVATE -Wall -Wextra -Werror)
set_target_properties(rn-tizen-simulator PROPERTIES BUILD_RPATH "$<TARGET_FILE_DIR:react-native-tizen>")
