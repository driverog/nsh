# nsh

`nsh` is a small Unix-like shell written in C. It supports external command execution plus a set of built-ins for job control, history, conditionals, and variables.

## Build and run

Requirements:
- CMake 3.11+
- A C compiler with C11 support

From the repository root:

```bash
cmake -B ./build
cmake --build ./build --target nsh -- -j 4
./build/nsh
```

## Prompt

The shell prompt is printed as:

```text
<user>@<host>:<cwd>
$ 
```

If the current working directory is inside your home directory, the home prefix is displayed as `~`.

## General syntax rules

- Tokens are space-separated.
- Multiple spaces are accepted.
- Text in double quotes is treated as a single argument.
- `#` starts a comment (ignored from that point to end of line).
- `~` expands to your home directory when used as a path token prefix.

## Features

### 1) Run external commands

Any executable in `PATH` can be run:

```bash
ls -la
pwd
cat file.txt
```

### 2) I/O redirection

- `>` redirect stdout (truncate/create)
- `>>` redirect stdout (append/create)
- `<` redirect stdin

Examples:

```bash
echo hello > out.txt
echo world >> out.txt
wc -l < out.txt
```

### 3) Pipes

Use `|` to connect stdout of one command to stdin of the next.

```bash
cat file.txt | grep error | wc -l
```

### 4) Command chaining

- `;` run commands sequentially
- `&&` run right side only if left side succeeds (exit status 0)
- `||` run right side only if left side fails (non-zero exit status)

Examples:

```bash
mkdir testdir ; cd testdir
grep needle file.txt && echo found
grep needle file.txt || echo not-found
```

### 5) Background execution and job control

Append `&` to run a command line in background:

```bash
sleep 30 &
```

Built-ins:

- `jobs` list active background jobs
- `fg [n]` wait for a background job in foreground
  - without argument: last background job
  - with argument: job index from `jobs`

Examples:

```bash
jobs
fg
fg 1
```

### 6) Command history (persistent)

History is persisted at:

```text
~/.nsh_history
```

- `history` prints recent commands (up to 10 retained)
- `again <n>` re-executes history entry number `n`

Examples:

```bash
history
again 3
```

Note: if a line starts with a leading space, it is not added to history.

### 7) Conditionals

`if` is a built-in with this structure:

```text
if <condition>
then
  <commands>
else
  <commands>
end
```

- `then` block is optional.
- `else` block is optional.
- Nested `if` is supported.

Single-line example:

```bash
if grep -q main main.c then echo yes else echo no end
```

You can also use helper built-ins:

- `true` returns success (`0`)
- `false` returns failure (`1`)

Example:

```bash
if true then echo ok else echo bad end
```

### 8) Shell variables

Variables are managed with built-ins:

- `set` show all variables
- `set <key> <value...>` set/update variable
- `get <key>` print variable value
- `unset <key>` remove variable

Examples:

```bash
set name nsh
get name
unset name
set
```

Command substitution in `set`:

```bash
set files `ls`
get files
```

The output of the command inside backticks is stored as the variable value.

### 9) Built-in commands summary

Commands executed directly by the shell process:

- `cd <dir>` change current directory
- `exit` leave the shell
- `fg [n]`
- `set [key value...]`
- `unset <key>`

Built-ins available in command execution/pipelines:

- `jobs`
- `history`
- `true`
- `false`
- `if ... then ... [else ...] end`
- `help [topic]`
- `get <key>`

## Help system

Inside `nsh`, run:

```bash
help
```

or:

```bash
help <topic>
```

Available topics include:

- `basic`
- `multi-pipe`
- `background`
- `spaces`
- `history`
- `ctrl+c`
- `chain`
- `if`
- `multi-if`
- `help`
- `variables`

## Signals

- `Ctrl+C` (`SIGINT`) is handled by the shell.
- Foreground command handling attempts graceful interruption and escalates when needed.
- Background jobs are managed separately.
