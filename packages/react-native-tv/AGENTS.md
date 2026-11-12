# Agent instructions

Read [`CLAUDE.md`](CLAUDE.md) first. It says what the package is, which reference
file covers which code, and how to check a change in an app. The rules below
apply to every change.

## Checks

```sh
bun run typecheck
bun run test
bun run check        # oxlint and oxfmt; bun run check:fix rewrites
bun run build        # dist/, the compiled JavaScript npm ships
```

## Writing

- English for code, comments, identifiers, docs and commit messages.
- Never write an em dash: not in code, comments, docs or commit messages. No en
  dash as a connector and no curly quotes either. Use a comma, a colon or a full
  stop.
- Commit messages are conventional and short: `fix(expo-patch): find the
  command table in Expo CLI 59`. The body, when there is one, says why.
- A pull request's title becomes its squash commit: write it for a user of the
  package.

## Comments

The default is no comment. Name things so the code says what it does.

- A doc comment (`/** */`) goes on exported API only, when the signature does
  not show its contract. One or two sentences. No `@param`, `@returns` or
  `@type`.
- A why-comment is for a constraint the code cannot express: a rule of Expo
  CLI, Metro, Node or a package manager. Name it and what breaks without it, in
  one or two lines.
- Never narrate your work, restate a line, add a section banner, write a file
  header essay, leave commented-out code or a TODO without an owner, or justify a
  type, a guard or a constant.
- When you edit a function, leave its comments no longer than you found them.
  Never delete one that records a workaround.

## Files

- One job per file, named after what it exports, in the naming style of the
  directory it joins.
- A file over 300 lines is split; one between 200 and 300 is split only at a
  natural seam. Generated files and JSON data are exempt.

## Trust boundaries

- Parse everything that comes from outside the process with zod: a
  `package.json`, the output of a child process, and what Expo CLI passes in.
  No `typeof` chains and no `as Record<string, unknown>`.
- Bound the size of untrusted input before you parse it.
- No `any`, and no `as` to silence the compiler.

## Reference first

Never implement a behaviour of Expo CLI, Metro or a package manager from
memory. Follow the [`reference-first`](.claude/skills/reference-first/SKILL.md)
skill: read the file in [`docs/reference/`](docs/reference/README.md), and when
the fact is missing, read it in the source or observe it, and add it, cited, in
the same change.

[`docs/decisions.md`](docs/decisions.md) holds the choices the package is built
on. Change one only by changing that file in the same change.

## Never commit

Build output, a packed tarball or anything under `node_modules/`.
