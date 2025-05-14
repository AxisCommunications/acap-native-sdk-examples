# Example guidelines

Main guiding principles

- focus on explaining a use case.
- the code structure should be easy to understand.
- the examples should aim at being short.

## Guiding examples

This repository has many ACAP application examples which makes it important to
keep a common structure for both maintenance and familiarity for users.

New examples must follow the general example format. Most examples are aligned,
but not all, therefore it's recommended to follow one of these guiding examples:

- [axparameter](./axparameter/)
  <!-- textlint-disable terminology -->
- [vapix](./vapix/)
  <!-- textlint-enable -->

## Example files and structure

The minimum example structure

```sh
example-name
├── app
│   ├── .c / .cpp / shell script
│   ├── LICENSE
│   ├── Makefile (*)
│   └── manifest.json
├── Dockerfile
└── README.md

(*) Makefile is not necessary when architecture equals `all`.
```

- **Don't use build scripts like `build.sh`**
  - Build instructions are seen directly from the example README and it's easy
    to make custom builds from command-line by adding e.g. a build flag to the
    `docker` command. This setup is clean and easy to automate for different
    users' needs.
- **Use general names, avoid versions**
  - Don't use versions in text that will need future updates, making
    maintenance hard.
  - For example, instead of ACAP Native SDK 12, use ACAP Native SDK.
- **Add copyright license**
  - The standard license is Apache 2.0
  - Copy LICENSE file and year in line `Copyright 2025 Axis Communications`.
  - All source code should have a copyright header.
- **Dockerfile**
  - Don't add proxy variables like `http_proxy` and `https_proxy` in the
    example. See the [Proxy](https://developer.axis.com/acap/develop/proxy)
    section for how to work with proxy in build time.

### README

A key to make a good example is to put effort in making the example name, title
and introduction as clear and concise as possible.

- Describe the use case.
- An image, graph or code snippet might be helpful in some cases.

After the introduction

- Reference the APIs used and point to API documentation.
- For larger examples it's useful to make an outline of what the program does
  to help the reader.

### Makefile

For C and C++ files, use as minimum the same amount of GCC warning options as
the other examples and `-Werror` to make them into build errors.

### C or C++

- C is preferred, it tends to make examples clearer, especially shorter ones.
- C++ could be used in larger more advanced examples.
- Use the language that makes the shortest and easiest to read example

#### C-code style

All C and C++ files are linted by clang and should also be autoformatted by
clang, see [LINT.md](./LINT.md) for more details.

- **General**
  - Assume knowledge in C, examples should not explain basic functionality.
- **libc or glib?**
  - Use standard libc as default.
  - Use glib when it saves time, lines, is safer or easier to read.
- **Functions**
  - The function name is very important, it should explain what is done.
  - Functions should strive to do *one* thing.
  - Don't use function descriptions.
  - Avoid functions with many arguments. Use `structs` instead for such cases.
- **Function prototypes**
  - Try to avoid them to make examples shorter.
  - Place main function last.
    Place functions without dependencies at the top
    If possible, no forward declarations within the file
- **Comments**
  - Don't repeat the code in comments.
  - Comments should explain **why** and the code show **how**.
  - Use `//` as in `// An explaining comment` for all comments.
- **Error handling**
    <!-- textlint-disable terminology -->
  - At errors, use function `panic()` as in the [vapix](./vapix/) example. This
    function logs to syslog and exit with 1 directly without cleanup.
    <!-- textlint-enable -->
  - The examples should not focus on error handling via error propagation.
    By crashing early errors are caught on first occurrence and avoids lengthy
    code not suitable for example code.
