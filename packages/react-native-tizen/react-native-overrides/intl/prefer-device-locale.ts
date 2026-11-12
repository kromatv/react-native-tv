interface LocaleService {
  supportedLocalesOf(locales: readonly string[]): string[];
}

/**
 * Makes the device's locale, as Hermes' own Intl resolves it, the default of
 * the services FormatJS adds, which otherwise default to the first locale
 * whose data loaded.
 */
export function preferDeviceLocale(): void {
  const device = new Intl.DateTimeFormat().resolvedOptions().locale;
  const services: LocaleService[] = [Intl.PluralRules, Intl.NumberFormat, Intl.RelativeTimeFormat];
  for (const service of services) {
    const [supported] = service.supportedLocalesOf([device]);
    if (supported !== undefined && '__defaultLocale' in service) {
      Reflect.set(service, '__defaultLocale', supported);
    }
  }
}
