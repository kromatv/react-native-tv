<!-- The title becomes the squash commit and its CHANGELOG.md line in every package it touches: conventional form, such as fix(input): map the Back key on Tizen 8. CONTRIBUTING.md, Releases, lists the types. -->

## What

<!-- What changes and why this way. -->

## How it was tested

<!-- A simulator, a TV (model, year and Tizen or webOS version), or tests only. -->

## Checks

- [ ] `bun run typecheck`, `bun run test` and `bun run check` pass
- [ ] A platform fact this relies on is in the package's `docs/reference/`, cited
- [ ] `docs/` changed with any behaviour a user can see
