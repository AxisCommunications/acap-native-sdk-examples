<!-- omit from toc -->
# Lint of codebase

A set of different linters test the codebase and these must pass in order to
get a pull request approved.

<!-- omit from toc -->
## Table of contents

<!-- ToC GFM -->

- [Linter types](#linter-types)
  - [super-linter](#super-linter)
  - [Custom linters](#custom-linters)
- [Linters in GitHub Action](#linters-in-github-action)
- [Run linters locally](#run-linters-locally)
  - [From command-line](#from-command-line)
  - [Visual Studio Code](#visual-studio-code)
- [Autoformat files according to linter rules](#autoformat-files-according-to-linter-rules)
  - [Visual Studio Code extensions](#visual-studio-code-extensions)
  - [Other IDEs](#other-ides)
  - [With super-linter container interactively](#with-super-linter-container-interactively)
    - [Run single tests](#run-single-tests)
    - [Autoformat files](#autoformat-files)
  - [Notes on clang-format (C/C++)](#notes-on-clang-format-cc)

<!-- /ToC -->

## Linter types

There are two categories of linters in this repository:

- The tool [super-linter](https://github.com/super-linter/super-linter) with
  syntax checks of e.g. Markdown, Dockerfile etc.
- Custom linters created for this repository.

### super-linter

For details on which linters that run and the settings, see the file
`.github/super-linter.env` and the
[super-linter](https://github.com/super-linter/super-linter) documentation.

### Custom linters

The custom linter consists of several smaller checks on e.g. example
application structure and terminology not covered by super-linter. See the
invoking file `.github/run-custom-linters` for what checks that are run.

## Linters in GitHub Action

When you create a pull request, GitHub Actions will run a set of linters for the
files in the change.

If any of the linters gives an error, this will be shown in the action
connected to the pull request.

## Run linters locally

In order to speed up development, it's recommended to run linters as part of
your local development environment.

### From command-line

To run all linters:

```sh
./run-linters --all
```

For more options, like running only one of the linters, see the help menu:

```sh
./run-linters --help
```

### Visual Studio Code

In Visual Studio Code it's also possible to run the linters as a task.

1. Go to menu `Terminal` > `Run Task`.
1. Choose one of the tasks available under `acap-native-sdk-examples:`.
1. A new shell is opened and runs the linters.

## Autoformat files according to linter rules

For some of the file types in this repository it's recommended to use a tool to
format files according to linter rules, especially C/C++ files where manual
formatting is not efficient, see [Notes on clang-format
(C/C++)](#notes-on-clang-format-cc).

The most common way is to use formatting tools for your editor. An option is to
use the linter tools themselves in the super-linter container to perform this
formatting, see section [With super-linter container
interactively](#with-super-linter-container-interactively).

### Visual Studio Code extensions

This repository has recommended extensions where some offer automatic
formatting, see the extension documentation for more info.

### Other IDEs

The configuration files are found in the top directory of this repository and
should be able to work with any editor formatting tool pointing out these files.

### With super-linter container interactively

It might be more convenient to run super-linter interactively, e.g. for faster
iteration of a single test or to [autoformat](#autoformat-files) files.

Another benefit is that the container contains the same setup including tool
versions as in GitHub Actions.

#### Run single tests

Run the container with your user to not change ownership of files.

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

#### Autoformat files

A few of the linters have automatic formatting.

```sh
docker run --rm \
  -u $(id -u):$(id -g) \
  -e USER \
  -v $PWD:/tmp/lint \
  -w /tmp/lint \
  --entrypoint /bin/bash \
  -it ghcr.io/super-linter/super-linter:slim-v6

# Autoformat C and C++ files (second line change all)
clang-format -i <path/to/file>
clang-format -i $(find . -regex '.*\.\(c\|cpp\|h\|cc\|C\|CPP\|c++\|cp\)$')

# Fix Markdown file errors
markdownlint -f <path/to/file>

# Fix Natural language / textlint remarks
textlint -c .textlintrc --fix .
```

### Notes on clang-format (C/C++)

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
container](#with-super-linter-container-interactively) to autoformat files.
