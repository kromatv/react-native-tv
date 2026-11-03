import { describe, expect, it } from 'vitest';
import { SDB_COMMAND } from './packet.ts';
import { SdbStream, type StreamTransport } from './sdb-stream.ts';

function transport(maxData = 4) {
  const sent: { command: number; arg0: number; arg1: number; data: Buffer }[] = [];
  const released: number[] = [];
  const link: StreamTransport = {
    maxData,
    send: (command, arg0, arg1, data) => sent.push({ command, arg0, arg1, data }),
    release: (localId) => released.push(localId),
  };
  return { link, sent, released };
}

async function open(link: StreamTransport): Promise<SdbStream> {
  const stream = new SdbStream('shell:x', 1, link);
  const opened = stream.opened(1000);
  stream.onOkay(0x700);
  await opened;
  return stream;
}

describe('SdbStream', () => {
  it('writes one packet at a time, each after the previous OKAY', async () => {
    const { link, sent } = transport(4);
    const stream = await open(link);

    const writing = stream.write(Buffer.from('abcdefgh'));
    expect(sent.filter((packet) => packet.command === SDB_COMMAND.WRTE)).toHaveLength(1);
    stream.onOkay(0x700);
    await new Promise((resolve) => setImmediate(resolve));
    stream.onOkay(0x700);
    await writing;

    expect(sent.map((packet) => packet.data.toString())).toEqual(['abcd', 'efgh']);
  });

  it('refuses to open when the TV closes first, and answers a close with one', async () => {
    const { link, sent, released } = transport();
    const stream = new SdbStream('shell:x', 1, link);
    const opened = stream.opened(1000);

    stream.onClose();

    await expect(opened).rejects.toThrow('refused shell:x');
    expect(sent).toEqual([]);
    expect(released).toEqual([1]);
  });

  it('fails a pending write when the TV closes the stream, and a later one at once', async () => {
    const { link, sent } = transport();
    const stream = await open(link);

    const writing = stream.write(Buffer.from('ab'));
    stream.onClose();

    await expect(writing).rejects.toThrow('during a write');
    await expect(stream.write(Buffer.from('cd'))).rejects.toThrow('closed shell:x');
    expect(sent.at(-1)).toMatchObject({ command: SDB_COMMAND.CLSE, arg0: 0, arg1: 0x700 });
  });

  it('hands every reader the connection failure', async () => {
    const { link } = transport();
    const stream = await open(link);

    const reading = stream.read(1000);
    stream.onFail(new Error('socket gone'));

    await expect(reading).rejects.toThrow('socket gone');
    await expect(stream.read()).rejects.toThrow('socket gone');
    await expect(stream.write(Buffer.from('x'))).rejects.toThrow('socket gone');
  });

  it('times out a read the TV never answers', async () => {
    const { link } = transport();
    const stream = await open(link);

    await expect(stream.read(20)).rejects.toThrow('went quiet');
  });

  it('keeps payloads that arrive before anyone reads, and acknowledges each', async () => {
    const { link, sent } = transport();
    const stream = await open(link);

    stream.onData(Buffer.from('one'));
    stream.onData(Buffer.from('two'));
    stream.onClose();

    expect((await stream.drain(100)).toString()).toBe('onetwo');
    expect(sent.filter((packet) => packet.command === SDB_COMMAND.OKAY)).toHaveLength(2);
  });
});
