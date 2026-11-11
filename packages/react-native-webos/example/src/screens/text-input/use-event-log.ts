import { useCallback, useRef, useState } from 'react';

const KEPT = 9;

/** One text input event, as a line of text. */
export interface LogLine {
  readonly id: number;
  readonly text: string;
}

/** The last few text input events, newest first. */
export function useEventLog(): readonly [readonly LogLine[], (text: string) => void] {
  const [lines, setLines] = useState<readonly LogLine[]>([]);
  const nextId = useRef(0);
  const add = useCallback((text: string) => {
    console.log(`[text-input] ${text}`);
    const line = { id: nextId.current++, text };
    setLines((previous) => [line, ...previous].slice(0, KEPT));
  }, []);
  return [lines, add];
}
