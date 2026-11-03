import { Directory, File, Paths } from 'expo-file-system';
import { attempt, check, CheckList, useChecks, type Check } from './check-list.tsx';

const SESSION = JSON.stringify({ token: 'abc', profile: 'living room' });

function sessionFile(): File {
  const directory = new Directory(Paths.document, 'rn-tizen');
  if (!directory.exists) directory.create({ intermediates: true });
  return new File(directory, 'session.json');
}

async function fileSystemChecks(): Promise<readonly Check[]> {
  return [
    await attempt('Paths.document and Paths.cache', () =>
      check(
        'Paths.document and Paths.cache',
        Paths.document.exists && Paths.cache.exists,
        `${Paths.document.uri} ${Paths.cache.uri}`,
      ),
    ),
    await attempt('writeSync, then exists on the next line', () => {
      const file = sessionFile();
      if (file.exists) file.delete();
      file.writeSync(SESSION);
      return check('writeSync, then exists on the next line', file.exists, file.uri);
    }),
    await attempt('await write() off the JavaScript thread', async () => {
      const file = sessionFile();
      file.delete();
      const pending = file.write(SESSION);
      const before = file.exists;
      await pending;
      return check('await write() off the JavaScript thread', file.exists, {
        existsBeforeAwait: before,
      });
    }),
    await attempt('await text() reads it back', async () => {
      const text = await sessionFile().text();
      return check('await text() reads it back', text === SESSION, text);
    }),
    await attempt('textSync, size and md5', () => {
      const file = sessionFile();
      return check(
        'textSync, size and md5',
        file.textSync() === SESSION && file.size === SESSION.length && file.md5?.length === 32,
        {
          size: file.size,
          md5: file.md5,
        },
      );
    }),
    await attempt('bytes and append', async () => {
      const file = new File(Paths.cache, 'append.txt');
      await file.write('ab');
      await file.write(new Uint8Array([99]), { append: true });
      const bytes = await file.bytes();
      return check(
        'bytes and append',
        file.textSync() === 'abc' && bytes.length === 3,
        file.textSync(),
      );
    }),
    await attempt('list, copy, move and delete', async () => {
      const directory = new Directory(Paths.cache, 'listing');
      if (directory.exists) directory.delete();
      directory.create();
      const first = directory.createFile('first.txt', null);
      first.writeSync('1');
      const copy = new File(directory, 'copy.txt');
      await first.copy(copy);
      const moved = new File(Paths.cache, 'moved.txt');
      if (moved.exists) moved.delete();
      copy.moveSync(moved);
      const names = directory
        .list()
        .map((entry) => entry.name)
        .sort((left, right) => left.localeCompare(right));
      const result = { names, moved: moved.exists, movedUri: copy.uri === moved.uri };
      directory.delete();
      return check(
        'list, copy, move and delete',
        names.join() === 'first.txt' && result.moved && !directory.exists,
        result,
      );
    }),
    await attempt('Paths.info and disk space', () => {
      const info = Paths.info(Paths.document.uri);
      return check(
        'Paths.info and disk space',
        info.exists && info.isDirectory === true && Paths.availableDiskSpace > 0,
        {
          info,
          availableMiB: Math.round(Paths.availableDiskSpace / 2 ** 20),
        },
      );
    }),
  ];
}

export function FileSystemScreen() {
  return <CheckList checks={useChecks('file-system', fileSystemChecks)} />;
}
