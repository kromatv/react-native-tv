import { describe, expect, it } from 'vitest';
import { parseVoiceCommand, parseVoiceSearch } from './voice-events.ts';

describe('parseVoiceCommand', () => {
  it('reads a skip with its signed seconds', () => {
    expect(parseVoiceCommand({ type: 'skip', seconds: -30 })).toEqual({
      type: 'skip',
      seconds: -30,
    });
  });

  it('reads a seek to a position in seconds', () => {
    expect(parseVoiceCommand({ type: 'seek', position: 4800 })).toEqual({
      type: 'seek',
      position: 4800,
    });
  });

  it('reads a command without arguments', () => {
    expect(parseVoiceCommand({ type: 'pause' })).toEqual({ type: 'pause' });
  });

  it('refuses a seek before the start', () => {
    expect(parseVoiceCommand({ type: 'seek', position: -1 })).toBeNull();
  });

  it('refuses an unknown command', () => {
    expect(parseVoiceCommand({ type: 'zoom' })).toBeNull();
  });
});

describe('parseVoiceSearch', () => {
  it('reads the fields the assistant filled', () => {
    expect(parseVoiceSearch({ utterance: 'search blade runner', title: 'blade runner' })).toEqual({
      utterance: 'search blade runner',
      title: 'blade runner',
    });
  });

  it('refuses a search without its utterance', () => {
    expect(parseVoiceSearch({ title: 'blade runner' })).toBeNull();
  });
});
