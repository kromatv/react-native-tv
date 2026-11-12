import { describe, expect, it } from 'vitest';
import { ninjaProgress } from './ninja-progress.ts';

describe('ninjaProgress', () => {
  it('reads the step counter of a Ninja status line', () => {
    expect(ninjaProgress('[12/480] Building CXX object host/CMakeFiles/a.o')).toBe('12/480');
  });

  it('ignores a line that is not a Ninja status line', () => {
    expect(ninjaProgress('ninja: no work to do.')).toBeNull();
  });
});
