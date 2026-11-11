import { describe, expect, it } from 'vitest';
import { FakeStdin } from './expo-stdin.fixtures.ts';
import { promptTizenTarget } from './tizen-prompt.ts';

const TARGETS = [
  { target: '10.0.0.7:26101', label: '10.0.0.7  The Frame  (used last)' },
  { target: 'simulator', label: 'Tizen simulator' },
];

function prompted() {
  const stdin = new FakeStdin();
  stdin.startExpo();
  let printed = '';
  const answer = promptTizenTarget(stdin, TARGETS, (text) => {
    printed += text;
  });
  return { stdin, answer, printed: () => printed };
}

describe('promptTizenTarget', () => {
  it("lists the targets, takes Expo's handler off meanwhile, and answers the one picked", async () => {
    const { stdin, answer, printed } = prompted();

    expect(printed()).toBe(
      '› Select a Tizen TV or the simulator\n  1  10.0.0.7  The Frame  (used last)\n  2  Tizen simulator\n  Esc  cancel\n',
    );
    stdin.press('9');
    stdin.press('a');
    stdin.press('2');

    expect(await answer).toBe('simulator');
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

    expect(await answer).toBe('10.0.0.7:26101');
    expect(stdin.listeners('data').filter((each) => each === stdin.handleKeypress)).toHaveLength(1);
  });
});
