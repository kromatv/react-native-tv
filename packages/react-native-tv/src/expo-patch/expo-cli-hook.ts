/** The module the patch makes Expo CLI load, which also marks a patched file as this package's. */
export const EXPO_CLI_EXTENSION = '@kromatv/react-native-tv/expo-cli';

const COMMAND_TABLE = 'const commands = {';
const ARGUMENT_PARSER = /^const args = \(0, _arg\(\)\.default\)\(\{$/gm;

const HOOK = `{
    let tvCommands = null;
    try {
        tvCommands = require.resolve('${EXPO_CLI_EXTENSION}', {
            paths: [
                process.cwd()
            ]
        });
    } catch {}
    if (tvCommands !== null) {
        try {
            require(tvCommands).extendExpoCli(commands);
        } catch (error) {
            console.warn('@kromatv/react-native-tv could not add its commands:', error);
        }
    }
}
`;

/** Whether the source of Expo CLI's entry loads this package's commands already. */
export function hasExpoCliHook(source: string): boolean {
  return source.includes(EXPO_CLI_EXTENSION);
}

/**
 * The source of Expo CLI's entry with this package's commands added to its
 * command table, or null when the file is not laid out as known. Expo CLI
 * builds the table, then parses the arguments and looks the command up in it,
 * so the hook goes between the two.
 */
export function addExpoCliHook(source: string): string | null {
  const parsers = [...source.matchAll(ARGUMENT_PARSER)];
  const [parser] = parsers;
  const table = source.indexOf(COMMAND_TABLE);
  if (parsers.length !== 1 || parser === undefined || table === -1 || table > parser.index)
    return null;
  return source.slice(0, parser.index) + HOOK + source.slice(parser.index);
}
