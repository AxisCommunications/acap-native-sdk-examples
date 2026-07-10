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
  - All source code should have a copyright header.
- **Dockerfile**
  - Don't add proxy variables like `http_proxy` and `https_proxy` in the
    example. See the [Proxy](https://developer.axis.com/acap/develop/proxy)
    section for how to work with proxy in build time.

### LICENSE

The standard license is MIT. The LICENSE file should contain (insert year):

```text
MIT License

Copyright (c) <PUBLISH_YEAR> Axis Communications AB

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### README

A key to make a good example is to put effort in making the example name, title
and introduction as clear and concise as possible.

- Describe the use case.
- Reference the APIs used and point to API documentation.
- An image, graph or code snippet might be helpful in some cases.

Use headers:

```text
# <Title describing use case>
## Project structure
## Application description
## Build the application
## Install and start the application
## Expected output
## License
```

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
