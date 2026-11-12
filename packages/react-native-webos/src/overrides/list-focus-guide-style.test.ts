import { describe, expect, it } from 'vitest';
import { listFocusGuideStyle } from '../../react-native-overrides/Libraries/Lists/list-focus-guide-style.ts';

describe('listFocusGuideStyle', () => {
  it("grows and shrinks like React Native's ScrollView for a list with no style", () => {
    expect(listFocusGuideStyle(undefined)).toEqual({ flexGrow: 1, flexShrink: 1 });
  });

  it('takes flex: 1 from the list, so the list fills what its parent leaves', () => {
    expect(listFocusGuideStyle({ flex: 1, backgroundColor: 'black' })).toEqual({
      flexGrow: 1,
      flexShrink: 1,
      flex: 1,
    });
  });

  it('lets the last style that sets a flex prop win, through nested arrays and gaps', () => {
    const style = [
      { flexGrow: 0, padding: 8 },
      null,
      false,
      [{ alignSelf: 'center' }, { flexGrow: 2 }],
    ];

    expect(listFocusGuideStyle(style)).toEqual({ flexGrow: 2, flexShrink: 1, alignSelf: 'center' });
  });

  it('keeps a flex prop the list unsets, as a flattened style does', () => {
    expect(listFocusGuideStyle({ flexShrink: undefined, flexBasis: 120 })).toEqual({
      flexGrow: 1,
      flexShrink: undefined,
      flexBasis: 120,
    });
  });

  it('leaves the size, margins and look to the list itself', () => {
    expect(listFocusGuideStyle({ height: 200, margin: 12, opacity: 0.5 })).toEqual({
      flexGrow: 1,
      flexShrink: 1,
    });
  });
});
