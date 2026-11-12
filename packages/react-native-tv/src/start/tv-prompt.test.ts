import { describe, expect, it } from 'vitest';
import { FakeStdin } from './expo-stdin.fixtures.ts';
import { MAX_CHOICES, promptTvTarget } from './tv-prompt.ts';

const LABELS = ['tizen  10.0.0.7  (used last)', 'webos  webOS simulator'];

function prompted(labels: readonly string[] = LABELS) {
  const stdin = new FakeStdin();
  stdin.startExpo();
  let printed = '';
  const answer = promptTvTarget(stdin, labels, (text) => {
    printed += text;
  });
  return { stdin, answer, printed: () => printed };
}

describe('promptTvTarget', () => {
  it("lists the targets, takes Expo's handler off meanwhile, and answers the one picked", async () => {
    const { stdin, answer, printed } = prompted();

    expect(printed()).toBe(
      '› Select a TV or a simulator\n  1  tizen  10.0.0.7  (used last)\n  2  webos  webOS simulator\n  Esc  cancel\n',
    );
    stdin.press('9');
    stdin.press('a');
    stdin.press('2');

    expect(await answer).toBe(1);
    expect(stdin.expoKeys).toEqual([]);
    expect(stdin.listeners('data')).toEqual([stdin.handleKeypress]);
  });

  it('answers nothing on Escape, and hands Ctrl+C on to Expo', async () => {
    const escaped = prompted();
    escaped.stdin.press('\u001b');
    expect(await escaped.answer).toBeNull();

    const interrupted = prompted();
    interrupted.stdin.press('\u0003');
    expect(await interrupted.answer).toBeNull();
    expect(interrupted.stdin.expoKeys).toEqual(['\u0003']);
  });

  it('does not put back a handler Expo put back itself', async () => {
    const { stdin, answer } = prompted();

    stdin.startExpo();
    stdin.press('1');

    expect(await answer).toBe(0);
    expect(stdin.listeners('data').filter((each) => each === stdin.handleKeypress)).toHaveLength(1);
  });

  it('offers as many targets as one key each can choose', () => {
    const many = Array.from({ length: MAX_CHOICES + 3 }, (_, index) => `target ${index + 1}`);
    const { printed } = prompted(many);

    expect(printed()).toContain(`  ${MAX_CHOICES}  target ${MAX_CHOICES}\n  Esc  cancel\n`);
  });
});
