export const USAGE = `Usage: rn-tv <command>

Commands
  run:<platform> [options]   Build the app, install it on a TV and launch it, with the options of that platform's own run
  <platform> [command]       That platform's own command line: rn-tv tizen simulator run is rn-tizen simulator run
  platforms                  List the TV platforms this app depends on
  expo-patch                 Teach this app's Expo CLI the same commands: npx expo run:<platform>

Options
  --help, -h                 Show this help
  --version, -v              Show the version
`;
