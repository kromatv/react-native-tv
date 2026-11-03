import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    include: ['src/**/*.test.ts'],
    environment: 'node',
    pool: 'threads',
    coverage: {
      provider: 'istanbul',
      reporter: ['text-summary', 'lcov'],
      reportsDirectory: './coverage',
      include: ['src/**/*.ts'],
      exclude: ['src/**/*.test.ts', 'src/**/*.fixtures.ts'],
      thresholds: { lines: 85, statements: 85, functions: 85, branches: 75 },
    },
  },
});
