import { existsSync } from 'node:fs';
import { copyFile, mkdir } from 'node:fs/promises';
import { homedir } from 'node:os';
import { join, resolve } from 'node:path';
import type { X509Certificate } from 'node:crypto';
import {
  COMMON_OPTIONS,
  parseCommandArgs,
  UsageError,
  type Command,
  type CommandContext,
} from '../cli/command.ts';
import { Reporter } from '../cli/reporter.ts';
import { closestMatch } from '../cli/suggest.ts';
import { FixableError } from '../errors.ts';
import { createAuthorCertificate } from '../signing/author-certificate.ts';
import { distributorDuids } from '../signing/distributor-duids.ts';
import { splitActionArgs } from '../simulator/action-args.ts';
import { stateDir } from '../state-dir.ts';
import { storePassword } from '../tizen/password-store.ts';
import { addProfile, setActiveProfile } from '../tizen/profiles-edit.ts';
import { profilesPath, readProfiles } from '../tizen/profiles-file.ts';
import { resolveSigning } from '../tizen/signing-identity.ts';

const OPTIONS = {
  author: {
    type: 'string',
    placeholder: '<file.p12>',
    description: 'import: the author certificate (password in RN_TIZEN_AUTHOR_PASSWORD)',
  },
  distributor: {
    type: 'string',
    placeholder: '<file.p12>',
    description: 'import: the distributor certificate (password in RN_TIZEN_DISTRIBUTOR_PASSWORD)',
  },
  ...COMMON_OPTIONS,
} as const;

interface Invocation {
  readonly positionals: readonly string[];
  readonly flags: ReturnType<typeof parseCommandArgs<typeof OPTIONS>>;
  readonly reporter: Reporter;
  readonly context: CommandContext;
}

const NAME = /^[A-Za-z0-9_.-]{1,64}$/;

function nameFrom(invocation: Invocation, action: string): string {
  const [name, ...extra] = invocation.positionals;
  if (name === undefined || extra.length > 0 || !NAME.test(name)) {
    throw new UsageError(
      `certificate ${action} needs one profile name: letters, digits, . _ -`,
      'certificate',
    );
  }
  return name;
}

/** Where Tizen Studio's Certificate Manager keeps Samsung's certificates, one directory per profile. */
function samsungCertificatesDir(env: NodeJS.ProcessEnv = process.env): string {
  return join(env.HOME ?? homedir(), 'SamsungCertificate');
}

function refuseExisting(directory: string): void {
  if (!existsSync(directory)) return;
  throw new FixableError(
    `${directory} already exists`,
    'Pick another name, so its certificates stay as they are.',
  );
}

const commonName = (certificate: X509Certificate | undefined, field: 'subject' | 'issuer') =>
  certificate?.[field]
    .split('\n')
    .find((line) => line.startsWith('CN='))
    ?.slice(3) ?? '?';
const day = (date: string) => new Date(date).toISOString().slice(0, 10);

function describe(certificate: X509Certificate | undefined): string {
  if (certificate === undefined) return 'none';
  const duids = distributorDuids(certificate);
  const listed = duids.length === 0 ? '' : `, DUIDs ${duids.join(' ')}`;
  return `${commonName(certificate, 'subject')}, issued by ${commonName(certificate, 'issuer')}, until ${day(certificate.validTo)}${listed}`;
}

async function list({ reporter }: Invocation): Promise<void> {
  const path = profilesPath();
  const file = readProfiles(path);
  if (file.profiles.length === 0) {
    reporter.line(`No signing profile in ${path}.`);
    reporter.note(
      'Import Samsung certificates with: rn-tizen certificate import <name> --author <p12> --distributor <p12>',
    );
    return;
  }
  const opened = await Promise.all(
    file.profiles.map((profile) =>
      resolveSigning({ profile: profile.name }, path).catch((error: unknown) => {
        if (error instanceof FixableError) return error;
        throw error;
      }),
    ),
  );
  file.profiles.forEach((profile, index) => {
    const active = profile.name === file.active ? reporter.paint('green', ' (active)') : '';
    reporter.line(`${reporter.paint('bold', profile.name)}${active}`);
    const resolved = opened[index];
    if (resolved === undefined || resolved instanceof FixableError) {
      reporter.field('', reporter.paint('yellow', resolved?.message ?? 'not opened'));
      return;
    }
    reporter.field('author', describe(resolved.identity.author.chain[0]));
    resolved.identity.distributors.forEach((distributor, slot) =>
      reporter.field(`distributor ${slot + 1}`, describe(distributor.chain[0])),
    );
  });
}

async function use(invocation: Invocation): Promise<void> {
  const name = nameFrom(invocation, 'use');
  await invocation.reporter.step(
    `Activate ${name}`,
    () => setActiveProfile(profilesPath(), name),
    () => profilesPath(),
  );
}

async function importProfile(invocation: Invocation): Promise<void> {
  const name = nameFrom(invocation, 'import');
  const { author, distributor } = invocation.flags;
  if (author === undefined || distributor === undefined) {
    throw new UsageError('certificate import needs --author and --distributor', 'certificate');
  }
  const directory = join(samsungCertificatesDir(), name);
  refuseExisting(directory);
  const { reporter, context } = invocation;
  const given = {
    author: resolve(context.cwd, author),
    distributor: resolve(context.cwd, distributor),
  };
  await reporter.step(
    'Open',
    () => resolveSigning(given),
    (resolved) => `distributor until ${day(resolved.distributor.validTo)}`,
  );
  await reporter.step(
    'Import',
    async () => {
      await mkdir(directory, { recursive: true });
      const importHalf = async (half: 'author' | 'distributor') => {
        const path = join(directory, `${half}.p12`);
        const password = join(directory, `${half}.pwd`);
        await copyFile(given[half], path);
        await storePassword(password, process.env[`RN_TIZEN_${half.toUpperCase()}_PASSWORD`] ?? '');
        return { path, password };
      };
      const [authorKey, distributorKey] = await Promise.all([
        importHalf('author'),
        importHalf('distributor'),
      ]);
      await addProfile(profilesPath(), { name, author: authorKey, distributors: [distributorKey] });
    },
    () => `${name}, now the active profile`,
  );
}

async function createAuthor(invocation: Invocation): Promise<void> {
  const name = nameFrom(invocation, 'create-author');
  const { reporter } = invocation;
  const directory = join(stateDir(), 'certificates', name);
  refuseExisting(directory);
  const author = await reporter.step(
    'Author certificate',
    async () => {
      await mkdir(directory, { recursive: true, mode: 0o700 });
      const created = await createAuthorCertificate({ directory, subject: { commonName: name } });
      await storePassword(join(directory, 'author.pwd'), created.password);
      return created;
    },
    (created) => created.path,
  );
  reporter.note(`Its password is stored for ${join(directory, 'author.pwd')}.`);
  reporter.note(
    `A retail Samsung TV refuses a self-signed author certificate; its author certificate comes from Samsung with the distributor one. Use this one for packages signed again before they are installed. Valid until ${day(author.certificate.validTo)}.`,
  );
}

const ACTIONS: Readonly<Record<string, (invocation: Invocation) => Promise<void>>> = {
  list,
  use,
  import: importProfile,
  'create-author': createAuthor,
};

/** `rn-tizen certificate`: lists, activates and imports signing profiles, and makes author certificates. */
export const certificateCommand: Command = {
  name: 'certificate',
  summary: 'List, activate or import signing profiles, or make an author certificate',
  options: OPTIONS,
  examples: [
    'rn-tizen certificate list',
    'rn-tizen certificate use MyTV',
    'rn-tizen certificate import MyTV --author author.p12 --distributor distributor.p12',
    'rn-tizen certificate create-author ci',
  ],
  async run(args, context) {
    const [first, ...rest] = args;
    const name = first === undefined || first.startsWith('-') ? 'list' : first;
    const action = ACTIONS[name];
    if (action === undefined) {
      const suggestion = closestMatch(name, Object.keys(ACTIONS));
      const hint = suggestion === null ? '' : `. Did you mean ${suggestion}?`;
      throw new UsageError(`Unknown certificate action ${name}${hint}`, 'certificate');
    }
    const { positionals, options } = splitActionArgs(name === first ? rest : args);
    const flags = parseCommandArgs('certificate', options, OPTIONS);
    const reporter = new Reporter({ verbose: flags.verbose ?? false });
    await action({ positionals, flags, reporter, context });
  },
};
