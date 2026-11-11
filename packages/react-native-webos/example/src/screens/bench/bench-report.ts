import { File } from 'expo-file-system';
import { useEffect, useState } from 'react';
import { BackHandler } from 'react-native';

const REPORT_URL = process.env.EXPO_PUBLIC_BENCH_REPORT_URL;
const STATUS_FILE = 'file:///proc/self/status';
const VM_RSS = /^VmRSS:\s+(\d+) kB$/m;
const VM_HWM = /^VmHWM:\s+(\d+) kB$/m;

export interface MemoryReading {
  readonly rssMb: number;
  readonly peakRssMb: number;
}

function statusKb(status: string, field: RegExp): number | null {
  const match = field.exec(status);
  return match?.[1] === undefined ? null : Number(match[1]);
}

function readMemory(): MemoryReading | null {
  try {
    const status = new File(STATUS_FILE).textSync();
    const rss = statusKb(status, VM_RSS);
    const peak = statusKb(status, VM_HWM);
    return rss === null || peak === null ? null : { rssMb: rss / 1024, peakRssMb: peak / 1024 };
  } catch (error) {
    console.warn(`[bench] cannot read ${STATUS_FILE}: ${String(error)}`);
    return null;
  }
}

/** Logs an event and posts it to `EXPO_PUBLIC_BENCH_REPORT_URL`, resolving with the server's answer. */
export async function report(event: string, data: object): Promise<string | null> {
  console.log(`[bench] ${event} ${JSON.stringify(data)}`);
  if (REPORT_URL === undefined) return null;
  try {
    const response = await fetch(`${REPORT_URL}/${event}`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data),
    });
    return await response.text();
  } catch (error) {
    console.warn(`[bench] ${event} not reported: ${String(error)}`);
    return null;
  }
}

/** Reports the first animation frame after the first mount; exits when the report server answers `exit`. */
export function useFirstFrameReport(): void {
  useEffect(() => {
    const frame = requestAnimationFrame(() => {
      void report('first-frame', {}).then((answer) => {
        if (answer === 'exit') BackHandler.exitApp();
      });
    });
    return () => cancelAnimationFrame(frame);
  }, []);
}

/** The process's resident memory, read from `/proc/self/status` and reported every `intervalMs`. */
export function useMemoryReport(intervalMs: number): MemoryReading | null {
  const [reading, setReading] = useState<MemoryReading | null>(null);
  useEffect(() => {
    const timer = setInterval(() => {
      const next = readMemory();
      setReading(next);
      if (next !== null) void report('memory', next);
    }, intervalMs);
    return () => clearInterval(timer);
  }, [intervalMs]);
  return reading;
}
