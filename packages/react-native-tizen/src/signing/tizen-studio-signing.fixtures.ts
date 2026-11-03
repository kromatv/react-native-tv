/** The files of a package Tizen Studio 10.0 signed with `tizen package -t tpk` and a test profile; its signatures are in `fixtures/`. */
export const FIXTURE_FILES: Readonly<Record<string, string>> = {
  'bin/App.dll': 'App.dll\n',
  'lib/libx.so': 'libx.so\n',
  'res/.hidden': 'hidden\n',
  'res/100%%.txt': 'percent\n',
  'res/Upper.txt': 'U\n',
  'res/a b+c&d.txt': 'plus\n',
  'res/deep/nested/file.json': '{}\n',
  'res/lower.txt': 'l\n',
  'res/semi;colon=eq@at.txt': 'reserved\n',
  'res/ünïcødé.txt': 'unicode\n',
  'shared/res/icon.png': 'icon\n',
  'tizen-manifest.xml':
    '<?xml version="1.0" encoding="utf-8"?>\n<manifest package="org.example.signing" version="1.0.0" api-version="9.0" xmlns="http://tizen.org/ns/packages">\n  <profile name="tv" />\n  <ui-application appid="org.example.signing" exec="App.dll" type="dotnet" />\n</manifest>\n',
};
