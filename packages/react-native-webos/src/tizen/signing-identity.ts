import { readFile } from 'node:fs/promises';
import type { X509Certificate } from 'node:crypto';
import { FixableError } from '../errors.ts';
import { Pkcs12PasswordError, readPkcs12 } from '../signing/pkcs12.ts';
import type { SigningIdentity } from '../signing/sign-package.ts';
import type { SigningKey } from '../signing/signature-xml.ts';
import { signingKeyFrom } from '../signing/signing-key.ts';
import { profilePassword } from './profile-password.ts';
import {
  profilesPath,
  readProfiles,
  type ProfileKey,
  type SecurityProfile,
} from './profiles-file.ts';

/** How to get Samsung's certificates, for a machine that has none. */
export const CERTIFICATE_HINT =
  "Create a Samsung certificate profile for your TV once, in Tizen Studio's Certificate Manager (Samsung Certificate Extension, the TV's DUID), or pass --author and --distributor with the .p12 files.";

/** Which certificates sign a package: a profile's, unless a flag names a `.p12` file. */
export interface SigningRequest {
  readonly profile?: string;
  readonly author?: string;
  readonly distributor?: string;
}

/** The keys a build signs with, and what to print about them. */
export interface ResolvedSigning {
  readonly name: string;
  readonly identity: SigningIdentity;
  readonly distributor: X509Certificate;
}

type Half = 'author' | 'distributor';

const PASSWORD_VARIABLE: Record<Half, string> = {
  author: 'RN_TIZEN_AUTHOR_PASSWORD',
  distributor: 'RN_TIZEN_DISTRIBUTOR_PASSWORD',
};

function chooseProfile(request: SigningRequest, path: string): SecurityProfile | null {
  if (request.author !== undefined && request.distributor !== undefined) return null;
  const file = readProfiles(path);
  const wanted = request.profile ?? file.active;
  if (wanted === null)
    throw new FixableError(`No active signing profile in ${path}`, CERTIFICATE_HINT);
  const profile = file.profiles.find((candidate) => candidate.name === wanted);
  if (profile !== undefined) return profile;
  const known = file.profiles.map((candidate) => candidate.name);
  const hint = known.length === 0 ? CERTIFICATE_HINT : `Profiles in ${path}: ${known.join(', ')}.`;
  throw new FixableError(`No signing profile named ${wanted}`, hint);
}

async function passwordFor(half: Half, key: ProfileKey | null): Promise<string> {
  const variable = process.env[PASSWORD_VARIABLE[half]];
  if (variable !== undefined && variable !== '') return variable;
  const stored = key === null ? null : await profilePassword(key.password);
  if (stored !== null) return stored;
  throw new FixableError(
    `No password for the ${half} certificate`,
    `Set ${PASSWORD_VARIABLE[half]}, or sign with a profile Tizen Studio stored the password of.`,
  );
}

async function openKey(half: Half, path: string, password: string): Promise<SigningKey> {
  const contents = await readFile(path)
    .then((file) => readPkcs12(file, password))
    .catch((error: unknown) => {
      if (error instanceof Pkcs12PasswordError) {
        throw new FixableError(
          `The ${half} password does not open ${path}`,
          `Check ${PASSWORD_VARIABLE[half]} or the profile.`,
        );
      }
      throw new FixableError(
        `Could not read the ${half} certificate ${path}`,
        error instanceof Error ? error.message : String(error),
      );
    });
  const key = signingKeyFrom(contents);
  const [leaf] = key.chain;
  if (leaf !== undefined && new Date(leaf.validTo).getTime() < Date.now()) {
    throw new FixableError(
      `The ${half} certificate in ${path} expired on ${leaf.validTo}`,
      CERTIFICATE_HINT,
    );
  }
  return key;
}

async function loadHalf(
  half: Half,
  flag: string | undefined,
  stored: ProfileKey | null,
): Promise<SigningKey> {
  const path = flag ?? stored?.path;
  if (path === undefined)
    throw new FixableError(`The profile has no ${half} certificate`, CERTIFICATE_HINT);
  return openKey(half, path, await passwordFor(half, flag === undefined ? stored : null));
}

/**
 * Opens the certificates a package is signed with, before anything is built:
 * the profile named, else the active one in Tizen Studio's `profiles.xml`, with
 * `--author` or `--distributor` replacing its halves.
 */
export async function resolveSigning(
  request: SigningRequest,
  path = profilesPath(),
): Promise<ResolvedSigning> {
  const profile = chooseProfile(request, path);
  const author = await loadHalf('author', request.author, profile?.author ?? null);
  const first = await loadHalf(
    'distributor',
    request.distributor,
    profile?.distributors[0] ?? null,
  );
  const second = request.distributor === undefined ? profile?.distributors[1] : undefined;
  const distributors =
    second === undefined ? [first] : [first, await loadHalf('distributor', undefined, second)];
  const [leaf] = first.chain;
  if (leaf === undefined)
    throw new FixableError('The distributor certificate is empty', CERTIFICATE_HINT);
  return {
    name: profile?.name ?? 'the given certificates',
    identity: { author, distributors },
    distributor: leaf,
  };
}
