import { describe, expect, it } from 'vitest';
import { tvProps, TvPropsError } from './tv-props.ts';

describe('tvProps', () => {
  it('keeps the shared fields as given and takes every other key as a block', () => {
    const props = tvProps({
      package: 'tv.example.app',
      version: 7,
      icon: './icon.png',
      locales: ['fr'],
      tvos: { androidTVRequired: true },
      webos: {},
    });

    expect(props.shared).toEqual({
      package: 'tv.example.app',
      version: 7,
      icon: './icon.png',
      locales: ['fr'],
    });
    expect([...props.blocks]).toEqual([
      ['tvos', { androidTVRequired: true }],
      ['webos', {}],
    ]);
  });

  it('takes no props as none', () => {
    expect(tvProps(undefined)).toEqual({ shared: {}, blocks: new Map() });
    expect(tvProps(null)).toEqual({ shared: {}, blocks: new Map() });
  });

  it.each([['props'], [3], [[]]])('refuses %j as props', (props) => {
    expect(() => tvProps(props)).toThrow(new TvPropsError('expected an object').message);
  });

  it.each([['KROMA'], [true], [['a']], [null]])('refuses %j where a block belongs', (value) => {
    expect(() => tvProps({ label: value })).toThrow(
      'Invalid @kromatv/react-native-tv props: "label" is neither a shared field (package, version, icon, locales) nor the block of a platform',
    );
  });
});
