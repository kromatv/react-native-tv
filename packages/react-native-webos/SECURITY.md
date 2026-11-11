# Security policy

## Reporting a vulnerability

Report it privately through GitHub: Security, Advisories, Report a vulnerability
([direct link](https://github.com/kromatv/react-native-tizen/security/advisories/new)).
Do not open a public issue or pull request for it.

Include the affected part, the version or commit, the steps to reproduce and what
an attacker gains. A proof of concept helps.

One person maintains this project, so there is no guaranteed fix window. Expect an
acknowledgement within a few days; the advisory thread settles the timeline.

## Supported versions

Only the latest release. Fixes ship in the next release, with no backports.

## Scope

In scope:

- the native runtime in `runtime/`, which runs on the TV: its networking, image
  loading and bundle loading
- the .NET shell in `shell/`
- the `rn-tizen` CLI in `src/`, which runs build tools, talks to TVs over sdb and
  listens for a development build's logs
- the packages it generates and signs
- the simulator in `simulator/`

Out of scope:

- the log stream of a development build. It is plain TCP to the address in the
  package's `react-native-tizen.json`, on purpose, for a TV on your own network.
  A `--release` build carries no log endpoint.
- bugs in Tizen, Samsung's firmware, Tizen Studio, Hermes, React Native or Expo
  themselves. Report those upstream. Tell us too if this package makes one
  reachable.
- findings that need a compromised development machine or physical access to the
  TV
- scanner output with no demonstrated impact

## Signing material

Your Samsung certificate and its passwords live in Tizen Studio's security
profiles and never in this repository or a generated `tizen/` project. A report
showing that `rn-tizen` leaks them into a package, a log or the cache is in
scope.
