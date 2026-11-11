import ssh2, { type SFTPWrapper } from 'ssh2';

interface OpenFile {
  readonly path: string;
  readonly chunks: { readonly offset: number; readonly data: Buffer }[];
}

interface SftpTarget {
  readonly files: Map<string, Buffer>;
  readonly transfers: ('sftp' | 'cat')[];
}

function assemble(file: OpenFile): Buffer {
  const size = file.chunks.reduce(
    (end, chunk) => Math.max(end, chunk.offset + chunk.data.length),
    0,
  );
  const bytes = Buffer.alloc(size);
  for (const chunk of file.chunks) chunk.data.copy(bytes, chunk.offset);
  return bytes;
}

/** Serves the SFTP subsystem of a fake TV: files opened for writing land in `tv.files`. */
export function acceptSftp(sftp: SFTPWrapper, tv: SftpTarget): void {
  const { OK, FAILURE } = ssh2.utils.sftp.STATUS_CODE;
  const open = new Map<number, OpenFile>();
  let nextHandle = 0;
  const fileFor = (handle: Buffer) =>
    handle.length === 4 ? open.get(handle.readUInt32BE(0)) : undefined;
  sftp.on('OPEN', (requestId, path) => {
    const handle = Buffer.alloc(4);
    handle.writeUInt32BE(nextHandle, 0);
    open.set(nextHandle, { path, chunks: [] });
    nextHandle += 1;
    sftp.handle(requestId, handle);
  });
  sftp.on('WRITE', (requestId, handle, offset, data) => {
    const file = fileFor(handle);
    file?.chunks.push({ offset, data: Buffer.from(data) });
    sftp.status(requestId, file === undefined ? FAILURE : OK);
  });
  sftp.on('CLOSE', (requestId, handle) => {
    const file = fileFor(handle);
    if (file !== undefined) {
      tv.files.set(file.path, assemble(file));
      tv.transfers.push('sftp');
      open.delete(handle.readUInt32BE(0));
    }
    sftp.status(requestId, file === undefined ? FAILURE : OK);
  });
}
