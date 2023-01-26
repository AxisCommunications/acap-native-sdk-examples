# Lint of code base

A set of different linters test the code base and these must pass in order to
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
editors may run it locally to lint the code base. For complete instructions and
guidance, see super-linter page for [running
locally](https://github.com/github/super-linter/blob/main/docs/run-linter-locally.md).

To run a number of linters on the code base from command line:

```sh
docker run --rm \
  -v $PWD:/tmp/lint \
  -e LINTER_RULES_PATH=/ \
  -e DOCKERFILE_HADOLINT_FILE_NAME=.hadolint.yml \
  -e MARKDOWN_CONFIG_FILE=.markdownlint.yml \
  -e YAML_CONFIG_FILE=.yamllint.yml \
  -e RUN_LOCAL=true \
  -e VALIDATE_BASH=true \
  -e VALIDATE_DOCKERFILE_HADOLINT=true \
  -e VALIDATE_JSON=true \
  -e VALIDATE_MARKDOWN=true \
  -e VALIDATE_SHELL_SHFMT=true \
  -e VALIDATE_YAML=true \
  github/super-linter:slim-v4
```

## Run super-linter interactively

It might be more convenient to run super-linter interactively. Run container and
enter command line:

```sh
docker run --rm \
  -v $PWD:/tmp/lint \
  -w /tmp/lint \
  --entrypoint /bin/bash \
  -it github/super-linter:slim-v4
```

Then from the container terminal, the following commands can lint the the code
base for different file types:

```sh
# Lint Dockerfile files
hadolint $(find -type f -name "Dockerfile*")

# Lint Markdown files
markdownlint .

# Lint YAML files
yamllint .

# Lint JSON files
eslint --ext .json .

# Lint shell script files
shellcheck $(shfmt -f .)
shfmt -d .
```

To lint only a specific file, replace `.` or `$(COMMAND)` with the file path.
