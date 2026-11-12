# Agent instructions

Read [`CLAUDE.md`](CLAUDE.md) first. It says what the package is, which reference
file covers which code, and how to check a change in the simulator or on a TV.
The rules below apply to every change.

## Checks

```sh
bun run typecheck
bun run test
bun run check        # oxlint and oxfmt; bun run check:fix rewrites
bun run build        # dist/, the compiled JavaScript npm ships
bun run test:runtime # the host's C++ tests (runtime/host/src/**/*.test.cpp and runtime/host/expo/**/*.test.cpp), in the simulator's VM
find simulator -name '*.sh' -print0 | xargs -0 shellcheck -x
```

CI runs the other five on Linux and macOS. On aarch64 Linux it compiles the
runtime and the simulator's launcher (`bun scripts/compile-simulator-runtime.ts`)
and runs `test:runtime`. That proves the C++ compiles and its tests pass: a
change in `runtime/` or `shell/` is still checked in the simulator or on a TV,
and the pull request says which.

## Writing

- English for code, comments, identifiers, docs and commit messages.
- Never write an em dash: not in code, comments, docs or commit messages. No en
  dash as a connector and no curly quotes either. Use a comma, a colon or a full
  stop.
- Commit messages are conventional and short: `fix(input): map the Back key on
  Tizen 8`. The body, when there is one, says why.
- A pull request's title becomes its squash commit and, for `feat`, `fix`,
  `perf`, `revert` and `docs`, its line in `CHANGELOG.md`: write it for a user
  of the package. `CONTRIBUTING.md`, Releases, has the types and what they bump.

## Comments

The default is no comment. Name things so the code says what it does.

- A doc comment (`/** */` in TypeScript, `///` in C++ and C#) goes on exported
  API only, when the signature does not show its contract. One or two sentences.
  No `@param`, `@returns` or `@type`.
- A why-comment is for a constraint the code cannot express: a rule of Tizen, EFL,
  Hermes, React Native, Expo or Samsung's store, or a workaround for a vendor bug.
  Name the platform and what breaks without it, in one or two lines.
- Never narrate your work, restate a line, add a section banner, write a file
  header essay, leave commented-out code or a TODO without an owner, or justify a
  type, a guard or a constant.
- When you edit a function, leave its comments no longer than you found them.
  Never delete one that records a workaround or a platform rule.

## Files

- One job per file, named after what it exports, in the naming style of the
  directory it joins.
- A file over 300 lines is split; one between 200 and 300 is split only at a
  natural seam. Generated files, vendored files and JSON data are exempt.

## Trust boundaries

- Parse everything that comes from outside the process with zod: `app.json` and
  other files, the network, the TV, the output of a child process. No `typeof`
  chains and no `as Record<string, unknown>`.
- Bound the size of untrusted input, such as a log line from the TV, before you
  parse it.
- No `any`, and no `as` to silence the compiler.

## Platform code: reference first

Never implement a platform behaviour from memory: a key name, a manifest field, an
Evas rule, an event payload. Follow the
[`reference-first`](.claude/skills/reference-first/SKILL.md) skill:

1. Read the file in [`docs/reference/`](docs/reference/README.md) for the area
   you change.
2. If the fact is missing, find it in the vendor's documentation, the SDK
   headers or the source of a project that ships on the platform.
3. Add it to the reference, cited, in the same change. A behaviour seen on
   hardware goes in with the TV's model, year and Tizen version.

[`docs/decisions.md`](docs/decisions.md) holds the choices the package is built
on. Change one only by changing that file in the same change.

## Never commit

Signing material (`.p12`, certificate profiles, their passwords), a generated
`tizen/` project, build output or anything under `node_modules/`.
