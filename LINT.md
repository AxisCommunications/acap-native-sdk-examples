# Lint of codebase

A set of different linters test the codebase and these must pass in order to
get a pull request approved.

## Linters in GitHub Action

When you create a pull request, a set of linters will run syntax and format
checks on different file types in GitHub Actions by making use of a tool called
[super-linter](https://github.com/super-linter/super-linter). If any of the
linters gives an error, this will be shown in the action connected to the pull
request.

In order to fasten up development, it's possible to run linters as part of your
local development environment.

## Run super-linter locally

Since super-linter is using a Docker image in GitHub Actions, users of other
editors may run it locally to lint the codebase. For complete instructions and
guidance, see super-linter page for [running
locally](https://github.com/super-linter/super-linter/blob/main/docs/run-linter-locally.md).

### Visual Studio Code

1. Go to menu `Terminal` > `Run Task`.
1. Choose task `super-linter: Run all linters`.
1. A new shell is opened and runs the linters.

### Command-line

To run all linters on the codebase from command-line, the same that would run
when a Pull Request is opened, use command:

```sh
docker run --rm \
  -v $PWD:/tmp/lint \
  -e RUN_LOCAL=true \
  --env-file ".github/super-linter.env" \
  ghcr.io/super-linter/super-linter:slim-v6
```

For more details which linters that run and the settings, see the file
`.github/super-linter.env`.

To only test one specific linter, e.g. lint Markdown, see the variable name in
`.github/super-linter.env` that in this case is `VALIDATE_MARKDOWN=true`.  Note
that you also need to specify the linter configuration file if there is one.
In this case `MARKDOWN_CONFIG_FILE=.markdownlint` together with the location
via `LINTER_RULES_PATH=/`. Then run the single linter with this command:

```sh
docker run --rm \
  -v $PWD:/tmp/lint \
  -e LINTER_RULES_PATH=/ \
  -e RUN_LOCAL=true \
  -e VALIDATE_MARKDOWN=true \
  -e MARKDOWN_CONFIG_FILE=.markdownlint.yml \
  ghcr.io/super-linter/super-linter:slim-v6
```

## Run super-linter interactively

It might be more convenient to run super-linter interactively. Run the container
with your user to not change ownership of files.

```sh
docker run --rm \
  -u $(id -u):$(id -g) \
  -e USER \
  -v $PWD:/tmp/lint \
  -w /tmp/lint \
  --entrypoint /bin/bash \
  -it ghcr.io/super-linter/super-linter:slim-v6
```

Then from the container terminal, the following commands can lint the codebase
for different file types:

```sh
# Lint Dockerfile files
hadolint $(find -type f -name "Dockerfile*")

# Lint Markdown files
markdownlint .

# Lint YAML files
yamllint .

# Lint JSON files
eslint --ext .json .

# Lint C and C++ files
# Note, manual formatting is discouraged, use clang-format to format these
# files. See more info in coming section.
clang-format --dry-run $(find . -regex '.*\.\(c\|cpp\|h\|cc\|C\|CPP\|c++\|cp\)$')

# Lint shell script files
shellcheck $(shfmt -f .)
shfmt -d .

# Lint Natural language / textlint
textlint -c .textlintrc .
```

To lint only a specific file, replace `.` or `$(COMMAND)` with the file path.

## Format files according to linter rules automatically

For some of the file types in this repository it's recommended to use a tool to
format files according to linter rules, especially C/C++ files where manual
formatting is not efficient.

The most common way is to use formatting tools for your editor. An option
is to use the linter tools themselves in super-linter container to perform this
formatting.

### Visual Studio Code extensions

This repository has recommended extensions where some offer automatic
formatting, see the extension documentation for more info.

#### clang-format (C/C++)

Super-linter checks that files are formatted according to the configuration
file `.clang-format` with a certain clang-format version. As long as the Visual
Studio Code extension adheres to this style, Pull Requests should not generate
linting errors.
The field *Likely compatible clang-format versions* marks versions that seem
compatible, but can't be guaranteed.

| super-linter | clang-format | Likely compatible clang-format versions |
| ------------ | ------------ | --------------------------------------- |
| slim-v6      | 17           | 15-17                                   |

If the setup is correct, the extension will automatically format the code
according to the configuration file on save and when typing.

If your OS or distribution doesn't have a compatible clang-format version to
install, use the approach in [Use the super-linter
container](use-the-super-linter-container) to autoformat files.

### Use the super-linter container

A few of the linters have automatic formatting.

```sh
docker run --rm \
  -u $(id -u):$(id -g) \
  -e USER \
  -v $PWD:/tmp/lint \
  -w /tmp/lint \
  --entrypoint /bin/bash \
  -it ghcr.io/super-linter/super-linter:slim-v6

# Auto-format C and C++ files (second line change all)
clang-format -i <path/to/file>
clang-format -i $(find . -regex '.*\.\(c\|cpp\|h\|cc\|C\|CPP\|c++\|cp\)$')

# Fix Markdown file errors
markdownlint -f <path/to/file>

# Fix Natural language / textlint remarks
textlint -c .textlintrc --fix .
```
