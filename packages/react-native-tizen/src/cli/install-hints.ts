interface InstallHint {
  readonly darwin: string;
  readonly other: string;
}

const HINTS: Readonly<Record<string, InstallHint>> = {
  cmake: {
    darwin: 'brew install cmake',
    other: 'Install CMake 3.24 or newer: https://cmake.org/download/',
  },
  ninja: { darwin: 'brew install ninja', other: 'Install Ninja: sudo apt install ninja-build' },
  dotnet: {
    darwin: 'brew install dotnet',
    other: 'Install the .NET SDK: https://dotnet.microsoft.com/download',
  },
  node: { darwin: 'brew install node', other: 'Install Node.js: https://nodejs.org' },
  clang: { darwin: 'brew install llvm lld', other: 'sudo apt install clang lld llvm' },
  ccache: { darwin: 'brew install ccache', other: 'sudo apt install ccache' },
  tar: { darwin: 'tar ships with macOS', other: 'sudo apt install tar' },
};

/** How to install a program this tool runs, on this machine's OS. */
export function installHint(program: string, platform: NodeJS.Platform = process.platform): string {
  const hint = HINTS[program];
  if (hint === undefined) return `Install ${program} and make sure it is on PATH.`;
  return platform === 'darwin' ? hint.darwin : hint.other;
}
