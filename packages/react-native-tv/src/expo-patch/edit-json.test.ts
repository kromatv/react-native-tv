import { describe, expect, it } from 'vitest';
import { removeJsonMember, setJsonString } from './edit-json.ts';

const MANIFEST = `{
  "name": "app",
  "dependencies": { "expo": "58.0.23",   "react": "19.3.1" },
    "patchedDependencies": {
        "@expo/cli@58.1.19": "patches/@expo%2Fcli@58.1.19.patch",
        "left-pad@1.3.0": "patches/left-pad@1.3.0.patch",
        "@expo/cli@58.1.20": "patches/@expo%2Fcli@58.1.20.patch"
    },
  "private": true
}
`;

describe('removeJsonMember', () => {
  const line = (key: string) =>
    MANIFEST.split('\n').find((candidate) => candidate.includes(`"${key}"`)) ?? '';

  it.each([['@expo/cli@58.1.19'], ['left-pad@1.3.0']])(
    'removes %s, a member with one after it, and leaves every other character',
    (key) => {
      expect(removeJsonMember(MANIFEST, ['patchedDependencies', key])).toBe(
        MANIFEST.replace(`${line(key)}\n`, ''),
      );
    },
  );

  it('removes the last member with the comma before it', () => {
    expect(removeJsonMember(MANIFEST, ['patchedDependencies', '@expo/cli@58.1.20'])).toBe(
      MANIFEST.replace(`,\n${line('@expo/cli@58.1.20')}`, ''),
    );
  });

  it('removes the object with its last member, and so on up to the root', () => {
    const one =
      '{\n  "a": 1,\n  "pnpm": {\n    "patchedDependencies": {\n      "x@1": "x.patch"\n    }\n  },\n  "z": 2\n}\n';

    expect(removeJsonMember(one, ['pnpm', 'patchedDependencies', 'x@1'])).toBe(
      '{\n  "a": 1,\n  "z": 2\n}\n',
    );
    expect(removeJsonMember('{ "only": { "x": 1 } }\n', ['only', 'x'])).toBe('{}\n');
    expect(removeJsonMember('{"a":1,"b":2}', ['b'])).toBe('{"a":1}');
  });

  it('gives the text back when the path names no member', () => {
    expect(removeJsonMember(MANIFEST, ['patchedDependencies', 'nothing'])).toBe(MANIFEST);
    expect(removeJsonMember(MANIFEST, ['resolutions', 'nothing'])).toBe(MANIFEST);
    expect(removeJsonMember(MANIFEST, ['name', 'nothing'])).toBe(MANIFEST);
    expect(removeJsonMember(MANIFEST, [])).toBe(MANIFEST);
  });
});

describe('setJsonString', () => {
  it('adds the object and its member after the last one, indented as the file is', () => {
    expect(
      setJsonString(
        '{\n    "name": "app",\n    "private": true\n}\n',
        'scripts',
        'postinstall',
        'x',
      ),
    ).toBe(
      '{\n    "name": "app",\n    "private": true,\n    "scripts": {\n        "postinstall": "x"\n    }\n}\n',
    );
    expect(setJsonString('{\r\n\t"name": "app"\r\n}', 'scripts', 'postinstall', 'x')).toBe(
      '{\r\n\t"name": "app",\r\n\t"scripts": {\r\n\t\t"postinstall": "x"\r\n\t}\r\n}',
    );
  });

  it('adds the member to an object that is there, in the style of its members', () => {
    const text =
      '{\n  "scripts": {\n    "start": "expo start",\n    "test":"vitest"\n  },\n  "name": "app"\n}\n';

    expect(setJsonString(text, 'scripts', 'postinstall', 'x')).toBe(
      '{\n  "scripts": {\n    "start": "expo start",\n    "test":"vitest",\n    "postinstall":"x"\n  },\n  "name": "app"\n}\n',
    );
    expect(
      setJsonString('{\n  "name": "app",\n  "scripts": {}\n}\n', 'scripts', 'postinstall', 'x'),
    ).toBe('{\n  "name": "app",\n  "scripts": {\n    "postinstall": "x"\n  }\n}\n');
  });

  it('replaces the value of a member that is there, and nothing else', () => {
    const text = '{ "scripts": { "postinstall": "patch-package", "start": "expo start" } }';

    expect(setJsonString(text, 'scripts', 'postinstall', 'patch-package && say "hi"')).toBe(
      '{ "scripts": { "postinstall": "patch-package && say \\"hi\\"", "start": "expo start" } }',
    );
  });

  it('keeps a file on one line on one line', () => {
    expect(setJsonString('{"name":"app"}', 'scripts', 'postinstall', 'x')).toBe(
      '{"name":"app","scripts":{"postinstall":"x"}}',
    );
    expect(setJsonString('{ "name": "app" }', 'scripts', 'postinstall', 'x')).toBe(
      '{ "name": "app", "scripts": { "postinstall": "x" } }',
    );
  });

  it('fills an empty root object with two spaces', () => {
    expect(setJsonString('{}\n', 'scripts', 'postinstall', 'x')).toBe(
      '{\n  "scripts": {\n    "postinstall": "x"\n  }\n}\n',
    );
  });

  it('gives null when the text has no root object or the holder is not an object', () => {
    expect(setJsonString('[]', 'scripts', 'postinstall', 'x')).toBeNull();
    expect(setJsonString('{ "scripts": "none" }', 'scripts', 'postinstall', 'x')).toBeNull();
  });
});
