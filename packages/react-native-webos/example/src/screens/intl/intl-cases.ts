/** One string an Intl call returned, named by the call. */
export interface IntlCase {
  readonly name: string;
  readonly value: string;
}

const AIRED = new Date(2025, 6, 24);
const BUILT = new Date(2026, 10, 3, 14, 5);
const LABELS = ['Zoulou', 'espagnol', 'Écossais', 'anglais', 'Allemand', 'Œnologue', 'Français'];
const TITLES = ['Zorro', 'Élite', 'avatar', 'Ångström', 'éclair', 'Eclair'];

function attempt(make: () => string): string {
  try {
    return make();
  } catch (error) {
    return `throws ${error instanceof Error ? error.message : String(error)}`;
  }
}

function dates(locale: string): [string, () => string][] {
  return [
    [
      `${locale} weekday day month`,
      () => AIRED.toLocaleDateString(locale, { weekday: 'short', day: 'numeric', month: 'short' }),
    ],
    [
      `${locale} day month year`,
      () => AIRED.toLocaleDateString(locale, { day: 'numeric', month: 'short', year: 'numeric' }),
    ],
    [
      `${locale} month year`,
      () => AIRED.toLocaleDateString(locale, { month: 'long', year: 'numeric' }),
    ],
    [
      `${locale} long date`,
      () => AIRED.toLocaleDateString(locale, { day: 'numeric', month: 'long', year: 'numeric' }),
    ],
    [
      `${locale} medium short`,
      () => BUILT.toLocaleString(locale, { dateStyle: 'medium', timeStyle: 'short' }),
    ],
    [
      `${locale} dateStyle short`,
      () => new Intl.DateTimeFormat(locale, { dateStyle: 'short' }).format(AIRED),
    ],
    [
      `${locale} clock`,
      () =>
        new Intl.DateTimeFormat(locale, {
          hour: '2-digit',
          minute: '2-digit',
          hour12: false,
        }).format(BUILT),
    ],
  ];
}

function relative(locale: string): [string, () => string][] {
  const auto = new Intl.RelativeTimeFormat(locale, { numeric: 'auto' });
  const short = new Intl.RelativeTimeFormat(locale, { numeric: 'auto', style: 'short' });
  return [
    [`${locale} days`, () => [-3, -1, 0, 1, 2].map((days) => auto.format(days, 'day')).join(' | ')],
    [
      `${locale} months years`,
      () =>
        [
          auto.format(-2, 'month'),
          auto.format(3, 'month'),
          auto.format(-1, 'year'),
          auto.format(2, 'year'),
        ].join(' | '),
    ],
    [
      `${locale} short`,
      () =>
        [short.format(-5, 'minute'), short.format(2, 'hour'), short.format(-1, 'day')].join(' | '),
    ],
  ];
}

function numbers(locale: string): [string, () => string][] {
  return [
    [
      `${locale} gigabytes`,
      () =>
        [7.8, 15.25, 1234.56]
          .map((value) => value.toLocaleString(locale, { maximumFractionDigits: 1 }))
          .join(' | '),
    ],
    [
      `${locale} count`,
      () =>
        [1064, 1234567.891].map((value) => new Intl.NumberFormat(locale).format(value)).join(' | '),
    ],
    [
      `${locale} decimal`,
      () =>
        new Intl.NumberFormat(locale, {
          minimumFractionDigits: 1,
          maximumFractionDigits: 1,
          useGrouping: false,
        }).format(12345.67),
    ],
    [
      `${locale} percent currency`,
      () =>
        [
          new Intl.NumberFormat(locale, { style: 'percent' }).format(0.42),
          new Intl.NumberFormat(locale, { style: 'currency', currency: 'EUR' }).format(9.9),
        ].join(' | '),
    ],
    [
      `${locale} plural`,
      () =>
        [0, 1, 1.5, 2, 1000000]
          .map((count) => new Intl.PluralRules(locale).select(count))
          .join(' '),
    ],
  ];
}

function text(locale: string): [string, () => string][] {
  return [
    [
      `${locale} collator base`,
      () => [...LABELS].sort(new Intl.Collator(locale, { sensitivity: 'base' }).compare).join(', '),
    ],
    [
      `${locale} base equal`,
      () => String(new Intl.Collator(locale, { sensitivity: 'base' }).compare('écran', 'Ecran')),
    ],
    [
      `${locale} localeCompare`,
      () => [...TITLES].sort((a, b) => a.localeCompare(b, locale)).join(', '),
    ],
    [
      `${locale} sentence case`,
      () =>
        ['aujourd’hui', 'écran', 'ﬁn']
          .map((word) => word.charAt(0).toLocaleUpperCase(locale) + word.slice(1))
          .join(' | '),
    ],
  ];
}

/** KROMA's Intl calls, in French and in English, each with the string it returned or how it threw. */
export function intlCases(): IntlCase[] {
  const calls = ['fr', 'en'].flatMap((locale) => [
    ...dates(locale),
    ...relative(locale),
    ...numbers(locale),
    ...text(locale),
  ]);
  return [
    ...calls.map(([name, make]) => ({ name, value: attempt(make) })),
    { name: 'upper case', value: attempt(() => 'déjà vu ß'.toUpperCase()) },
    { name: 'normalize NFD', value: attempt(() => String('é'.normalize('NFD').length)) },
  ];
}
