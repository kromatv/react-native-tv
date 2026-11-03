# Signing a Tizen package

How a `.tpk` is signed so a Samsung TV installs it: which files the signatures
cover, in what order and under which URIs, the XML around them, which
certificates the TV takes, and how Tizen Studio stores them. Checked on
2026-11-03.

| Tag | Meaning |
|---|---|
| **D** | Documented: Tizen's docs, the W3C spec Tizen follows, an RFC, or the source of Tizen's validator and installer (the line is cited) |
| **O** | Observed with Tizen Studio 10.0 on an arm64 Mac (`tizen package`, `tizen security-profiles`, its files) |
| **H** | Seen on hardware: GQ75LS03DAUXZG, 2024 Frame, Tizen 9.0, armv7, Developer mode, a Public-level Samsung profile. An install refused prints `install failed[<n>, <code>], reason: <reason>` ([sdb-protocol.md](sdb-protocol.md) section 4); the reasons below are quoted from it |

## 1. What a signed package holds

| Fact | Tag | Source |
|---|---|---|
| Tizen's signing "follows the specification of the XML Digital Signatures for Widgets specified by W3C". | D | [td-native-certs] 119-122, [w3c-digsig] |
| A package needs an author signature (`author-signature.xml`) and a distributor signature (`signature1.xml`); without either the installer stops: "Author or distribuor signature is missing." | D | [ai-cert] 229-234 |
| "First distributor signature sets the privilege level". `signature1.xml` under an unknown root fails; `signature2.xml` and later under an unknown root are disregarded. A profile holds "multiple distributor certificates up to two". | D | [ai-cert] 184-189, [cs-base] 262-269, [td-profile] 18 |
| A TPK is checked like a widget, on the unpacked directory; the validator has no TPK or WGT case. | D | [ai-step] 66-68, [cs-base] |

## 2. Which files a signature covers

| Fact | Tag | Source |
|---|---|---|
| The validator walks the package: every regular file and every symlink must be referenced; directories are not. At the root only, it skips `author-signature.xml` when checking the author signature, and every file matching `^signature[1-9][0-9]*\.xml` for both. | D | [cs-ref] 47, 198-218 |
| So a distributor signature references `author-signature.xml`, which Tizen Studio lists among the others. | D, O | [cs-ref] 198-201 |
| A file no signature references fails the install: "Invalid file reference. An unsinged file was found." (the typo is the platform's). | D, H | [cs-base] 463 |
| Tizen Studio leaves out of the references every file named like a signature, at any depth. A package with `res/signature1.xml` it signed was refused by the TV: `Check certificate error : :Invalid file reference. An unsinged file was found.:<-5>`. The same package signed with that file referenced installed. | D, O, H | [ide-signer] 850-881 |
| A changed file fails with `Check certificate error : :Invalid signature. Signed with wrong key, changed signature file or changed package file.:<-4>`. | H | |

## 3. Order and URIs

| Fact | Tag | Source |
|---|---|---|
| The validator keeps references in a set, so their order is not checked. | D | [cs-sigdata] 44 |
| Tizen Studio sorts the relative paths with Java's string order, by UTF-16 code unit: `res/Upper.txt` before `res/a b+c&d.txt` before `res/lower.txt`, and `res/ünï…` after `res/semi…`. | D, O | [ide-signer] 1151 |
| Each path is escaped: the characters `% <>#{}\|\^~[]` `` ` `` `;/?@=&$!*:+` become `%XX` in upper-case hex, and non-ASCII characters their UTF-8 bytes. So `bin/App.dll` is `bin%2FApp.dll` and `res/a b+c&d.txt` is `res%2Fa%20b%2Bc%26d.txt`. | D, O | [ide-uri] 95-101 |
| The validator decodes a reference holding a `%` before it compares it with the files. | D | [cs-ref] 70-80 |

## 4. The signature document

| Fact | Tag | Source |
|---|---|---|
| Tizen Studio 10.0 writes exclusive canonicalization, RSA-SHA512 and SHA-512 digests. The 2018 source of its signer writes RSA-SHA256 and SHA-256; the shipped one has moved on. The validator has no list of algorithms and refuses only MD5. | O, D | [ide-signer] 584-585, 1182, [cs-xmlsec] 353-357 |
| After the file references comes one reference to `#prop`, transformed by C14N 1.1. `Object Id="prop"` holds `dsp:Profile` (`...widgets-digsig#profile`), `dsp:Role` (`#role-author` or `#role-distributor`) and an empty `dsp:Identifier`. | O | |
| The validator requires that profile URI, a role that matches the file name, and a reference to each `Object`; it does not check the identifier's value. | D | [cs-base] 95-131 |
| `SignedInfo` and the `prop` object are written already canonical, one element per line, so signing the bytes as written with the `SignedInfo` namespace declared is what exclusive canonicalization would hash. Base64 is wrapped at 76 columns, and the file ends at `</Signature>` with no newline. | O | |
| `KeyInfo` holds the PKCS#12 file's chain, leaf first: two certificates for Samsung's profile, three for a test file that also held its root. | O | |

## 5. The certificates a Samsung TV takes

| Fact | Tag | Source |
|---|---|---|
| The root of the author's chain must be in the "tizen developer" domain, and a distributor's must not be. | D | [cs-base] 248-261 |
| Samsung's Certificate Extension issues both halves. On this machine: author `<name> ← Samsung VD Author CA ← Tizen Developers Root Class`, distributor `TizenSDK ← VD DEVELOPER Public CA Class ← VD DEVELOPER Public Root Class`, each valid one year. | O | |
| "Samsung also disallows unauthorized applications to be installed on its Tizen devices. In such cases, you must get an author certificate and a distributor certificate from the device vendor." | D | [td-native-certs] 109-113 |
| A self-signed author with Samsung's distributor was refused: `Check certificate error : :Invalid certificate chain with certificate in signature.:<-3>`. | H | |
| An author from Tizen Studio's own developer CA (`Tizen Developers CA ← Tizen Developers Root`, both expiring 2027-01-01) passed the chain check and was refused as an update of an app another author signed: `install failed[118, -11], reason: Author certificate not match :`. | H | |
| Updates must be signed with the same author certificate as the original. | D | [tizen-tv-platform.md](tizen-tv-platform.md) section 1.3 |

## 6. PKCS#12 files and profiles

| Fact | Tag | Source |
|---|---|---|
| Samsung's `.p12` files encrypt the key with `pbeWithSHAAnd3-KeyTripleDES-CBC`, the certificates with `pbeWithSHAAnd40BitRC2-CBC`, and carry a SHA-1 MAC. Keys and IVs come from RFC 7292's key derivation. | O, D | [rfc7292] appendix B |
| `MacData` is optional: a file carries it only in password integrity mode. Its MAC is an HMAC over the contents of the authSafe's Data (without the OCTET STRING's tag and length), keyed by RFC 7292's derivation with ID 3 from the password, `macSalt` and `iterations`, which defaults to 1. | D | [rfc7292] sections 4 and 5.1 step 5B, appendix B.3 |
| Without a MAC, the only check on a password is the CBC padding of what it decrypts. A wrong key yields valid PKCS #7 padding about once in 256 tries, and the bytes that come out then fail to parse, as `asn1 encoding routines::wrong tag` from OpenSSL. | O, D | [rfc8018] section 6.1.2 step 5 |
| Node's OpenSSL 3 has no RC2: `createDecipheriv('rc2-40-cbc', ...)` fails with "unsupported". RC2 is RFC 2268. | O, D | [rfc2268] |
| `tizen security-profiles add` refuses a `.p12` whose key is under PBES2 and AES-256 ("Invalid file or password"), and the JDK 8u322 it ships cannot check an HMAC-SHA256 MAC ("HmacPBESHA256 not available"). A file with 3DES for key and certificates and a SHA-1 MAC is accepted. | O | |
| `profiles.xml` (version 3.1) names the active profile in `<profiles active=...>`. Each `<profile>` has `profileitem`s: `distributor="0"` the author, `"1"` and `"2"` the distributors, `key` the `.p12` path, `password` the password itself or the path of a `.pwd` file. | O | |
| On macOS no `.pwd` file is written: the password is a generic password in the login keychain, service `certificate-manager`, account the `.pwd` path. `security -i` reads an `add-generic-password` command from stdin, its quoted arguments escaped with `\`, so a password stored that way never reaches a process's arguments. | O | |
| The data directory is the `TIZEN_SDK_DATA_PATH` line of `<tizen-studio>/sdk.info`, `~/tizen-studio-data` by default. | O | |

## 7. Checked byte for byte

- The example's `.tpk`, re-signed with the same Samsung profile by this package
  and by `tizen package -t tpk -s <profile>`: `author-signature.xml` and
  `signature1.xml` are identical, and so are the zip's entries and their
  order (O, 2026-11-03).
- A `.wgt` (`config.xml`, `index.html`, an icon, `js/a b/x+y.js`) re-signed
  by `tizen package -t wgt -s <profile>` and by this package: identical. The
  references, escaping and XML are the ones of a `.tpk` (O).
- A fixture whose names hold spaces, `+`, `&`, `%`, `;`, `=`, `@`, non-ASCII,
  a dotfile, nested directories and both cases: identical (O). It is kept in
  `src/signing/fixtures/`, and its test hands back Tizen Studio's own
  signature values only when they verify against the bytes this code signs,
  so no private key is needed.

## 8. What this package does

- References every file but the root signatures, as the validator does
  (section 2); for every other package the output is Tizen Studio's.
- Reads PKCS#12 in TypeScript: 3DES and RC2 with RFC 7292's derivation, and
  PBES2 with AES; writes 3DES with a SHA-1 MAC, the form Tizen Studio opens.
- Checks the MAC before it decrypts anything, so a wrong password is refused
  for certain. In a file without one, anything that fails to parse after
  decryption is taken for a wrong password too (section 6).
- Signs with a profile from `profiles.xml`, its passwords from the profile, the
  `.pwd` file or the macOS keychain, or with `.p12` files given on the command
  line. `rn-tizen certificate create-author` makes a self-signed author
  certificate for packages signed again before they are installed; section 5
  leaves a TV only Samsung's.

## Unknowns

- Which roots a Samsung TV counts as the developer domain: its fingerprint
  list is not published.
- Where Tizen Studio keeps a profile's password on Linux and Windows.
- What the empty `ca` and `rootca` attributes of a `profileitem` change.

## Sources

The Tizen sources were read at cert-svc `af5a3fe` (2026-05-22), app-installers
`2cd0753` (2026-11-03) and common-eplugin `5a87a6f` (branch `tizen_studio`,
2018-01-11, the newest published source of Tizen Studio's signer).

[w3c-digsig]: https://www.w3.org/TR/2013/REC-widgets-digsig-20130418/
[td-native-certs]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/native/guides/concepts/signing-certificates.md
[td-profile]: https://github.com/Samsung/tizen-docs/blob/master/docs/application/dotnet/guides/concepts/signing-certificates/managing-certificate-profile.md
[cs-ref]: https://git.tizen.org/cgit/platform/core/security/cert-svc/tree/src/vcore/ReferenceValidator.cpp
[cs-base]: https://git.tizen.org/cgit/platform/core/security/cert-svc/tree/src/vcore/BaseValidator.cpp
[cs-sigdata]: https://git.tizen.org/cgit/platform/core/security/cert-svc/tree/src/vcore/SignatureData.h
[cs-xmlsec]: https://git.tizen.org/cgit/platform/core/security/cert-svc/tree/src/vcore/XmlsecAdapter.cpp
[ai-cert]: https://git.tizen.org/cgit/platform/core/appfw/app-installers/tree/src/common/certificate_validation.cc
[ai-step]: https://git.tizen.org/cgit/platform/core/appfw/app-installers/tree/src/common/step/security/step_signature.cc
[ide-signer]: https://git.tizen.org/cgit/sdk/ide/common-eplugin/tree/org.tizen.common.sign/src/org/tizen/common/sign/signer/TizenIncrementalSigner.java?h=tizen_studio
[ide-uri]: https://git.tizen.org/cgit/sdk/ide/common-eplugin/tree/org.tizen.common/src/org/tizen/common/util/URIEscapeUtil.java?h=tizen_studio
[rfc7292]: https://www.rfc-editor.org/rfc/rfc7292
[rfc2268]: https://www.rfc-editor.org/rfc/rfc2268
[rfc8018]: https://www.rfc-editor.org/rfc/rfc8018
