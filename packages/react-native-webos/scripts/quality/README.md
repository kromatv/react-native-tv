# Quality metrics beyond SonarCloud

`ub-mode.sh` runs three of Robert C. Martin's tools on `src/`:
[mutator](https://github.com/unclebob/mutator),
[dryer](https://github.com/unclebob/dryer) and
[crapper](https://github.com/unclebob/crapper). Each answers a question
SonarCloud does not:

| Tool | What it reports | What SonarCloud shows instead |
| --- | --- | --- |
| mutator | Changes each covered operator, literal and condition, reruns the tests, and lists every change no test noticed | Coverage: whether a line ran, not whether a test checked what it did |
| dryer | Functions whose structure matches once local names and literals are set aside, from 4 lines | Copy-paste duplication from about 10 lines |
| crapper | CRAP per function: complexity weighed by the coverage of that function | Complexity and coverage as separate numbers |

None of them is a required check. The `Quality metrics` workflow runs them
every Monday, on demand (Actions, Quality metrics, Run workflow) and on a pull
request that changes this directory or the workflow, and puts the surviving
mutants, the worst CRAP scores and the duplicates in the run's summary. Every
report is in the run's `ub-mode` artifact. Mutation testing takes most of the
run: about 20 minutes on 12 cores, longer on a runner.

## Running it locally

```sh
scripts/quality/ub-mode.sh                    # coverage, crap, dry, mutate
scripts/quality/ub-mode.sh coverage mutate    # some steps only
scripts/quality/ub-mode.sh summary            # the Markdown the workflow prints
```

The first run clones the three tools, at the commits the script pins, into
`.cache/ub-mode` and installs each in its own virtualenv. They need Python 3.11
or newer: point `UB_MODE_PYTHON` at one if `python3` is older. `UB_MODE_HOME`
uses tools already cloned elsewhere.

Reports go to `.metrics/ub-mode`: `mutate.txt`, `crap.txt` and `dry.txt`. A
mutant runs the whole vitest suite, which takes about a second, because the
test that covers a file is not always next to it.

## Reading the results

A surviving mutant is a change to the code that every test still passes. Most
are a missing assertion: write the test that fails on it. A few are
equivalent, such as `>` to `>=` where the two values can never be equal; leave
those.

A duplicate is worth removing when the copies must change together, like the
same parser in four files. Two functions that only share a shape are not.
