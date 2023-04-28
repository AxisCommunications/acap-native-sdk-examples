# Lint of codebase

A set of different linters test the codebase and these must pass in order to
get a pull request approved.

## Linters in GitHub Action

When you create a pull request, a set of linters will run syntax and format
checks on different file types in GitHub actions by making use of a tool called
[super-linter](https://github.com/github/super-linter). If any of the linters
gives an error, this will be shown in the action connected to the pull request.

In order to fasten up development, it's possible to run linters as part of your
local development environment.

## Run super-linter locally

Since super-linter is using a Docker image in GitHub Actions, users of other
editors may run it locally to lint the codebase. For complete instructions and
guidance, see super-linter page for [running
locally](https://github.com/github/super-linter/blob/main/docs/run-linter-locally.md).

To run a number of linters on the codebase from command-line:

```sh
docker run --rm \
  -v $PWD:/tmp/lint \
  -e LINTER_RULES_PATH=/ \
  -e DOCKERFILE_HADOLINT_FILE_NAME=.hadolint.yml \
  -e MARKDOWN_CONFIG_FILE=.markdownlint.yml \
  -e YAML_CONFIG_FILE=.yamllint.yml \
  -e RUN_LOCAL=true \
  -e VALIDATE_BASH=true \
  -e VALIDATE_CLANG_FORMAT=true \
  -e VALIDATE_DOCKERFILE_HADOLINT=true \
  -e VALIDATE_JSON=true \
  -e VALIDATE_MARKDOWN=true \
  -e VALIDATE_SHELL_SHFMT=true \
  -e VALIDATE_YAML=true \
  github/super-linter:slim-v5
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
  -it github/super-linter:slim-v5
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
clang-format --dry-run $(find . -regex ".*\.[h|c|cpp]")

# Lint shell script files
shellcheck $(shfmt -f .)
shfmt -d .
```

To lint only a specific file, replace `.` or `$(COMMAND)` with the file path.

## Format files according to linter rules automatically

For some of the file types in this repository it's recommended to use a tool to
format files according to linter rules, especially C/C++ files where manual
formatting is not efficient.

The most common way is to use formatting tools for your editor. An option
is to use the linter tools themselves in super-linter container to perform this
formatting.

### VS Code extensions

This repository has recommended extensions where some offer automatic
formatting, see the extension documentation for more info.

#### clang-format (C/C++)

The linter checks that files are formatted according to the configuration file
`.clang-format` of version 12. As long as the extension adheres to this style,
Pull Requests should not generate lint errors.

If the setup is correct, the extension will automatically format the code
according to the configuration file on save and when typing.

See installation instructions below.

### Use the super-linter container

A few of the linters have automatic formatting.

```sh
docker run --rm \
  -u $(id -u):$(id -g) \
  -e USER \
  -v $PWD:/tmp/lint \
  -w /tmp/lint \
  --entrypoint /bin/bash \
  -it github/super-linter:slim-v5

# Auto-format C and C++ files (second line change all .c,.h and .cpp files
clang-format -i <path/to/file>
clang-format -i $(find . -type f -regex ".*\.[ch]p\{0,2\}")

# Fix Markdown file errors
markdownlint -f <path/to/file>
```
