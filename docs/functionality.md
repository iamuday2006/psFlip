# CLI Tool Functionality

This document explains the behavior of each function in `psFlip.cpp` and how the tool translates simple shell-style commands into PowerShell equivalents.

## Overview

The program reads a single line of user input from `stdin`, translates recognized shell command names and flags into PowerShell equivalents, supports chaining commands with `&&`, and executes the final translated command using PowerShell.

## Why this exists and why use it

This tool is intended to make common shell-style commands usable on Windows PowerShell without rewriting them manually. It is useful when you want a quick compatibility layer for commands like `ls`, `pwd`, `grep`, and simple `&&` chains. The implementation is intentionally minimal to keep translation logic easy to understand and to preserve unknown commands when no mapping exists.

The functions exist so the process is broken into clear steps:

- `getUserInput()` reads the input line.
- `splitByAnd()` handles command chaining.
- `trim()` and `tokenize()` prepare each part for parsing.
- `translateOne()` performs command and flag translation.
- `executeCommand()` runs the translated PowerShell command.

This separation makes the code easier to extend later and isolates each behavior for debugging.

## What changed in the current implementation

The current code was updated so that `translate()` now accepts the whole raw user input, not just a single command and argument list. It now:

- splits the input using `&&`,
- trims each command fragment,
- tokenizes each fragment,
- translates each command and its args,
- joins translated segments with `; ` for PowerShell execution.

This change ensures chained commands like `ls -a && pwd` are translated in one pass instead of only translating the first command.

## Data Maps

### `commandMap`

A two-dimensional array mapping shell-style commands to PowerShell commands:

- `ls` -> `Get-ChildItem`
- `pwd` -> `Get-Location`
- `cd` -> `Set-Location`
- `cat` -> `Get-Content`
- `touch` -> `New-Item`
- `mkdir` -> `New-Item -ItemType Directory`
- `rm` -> `Remove-Item`
- `cp` -> `Copy-Item`
- `mv` -> `Move-Item`
- `find` -> `Get-ChildItem`
- `grep` -> `Select-String`
- `which` -> `Get-Command`
- `echo` -> `Write-Output`
- `clear` -> `Clear-Host`
- `ps` -> `Get-Process`
- `kill` -> `Stop-Process`
- `curl`, `wget` -> `Invoke-WebRequest`
- `tar`, `git`, `npm`, `node`, `python`, `pip`, `docker` are passed through unchanged.

### `flagMap`

A two-dimensional array mapping common shell-style flags to PowerShell equivalents:

- `-a` -> `-Force`
- `-r` -> `-Recurse`
- `-f` -> `-Force`
- `-rf` -> `-Recurse -Force`
- `-fr` -> `-Force -Recurse`
- `-i` -> `-Confirm`
- `-v` -> `-Verbose`
- `--help` -> `-?`
- `--force` -> `-Force`
- `--recursive` -> `-Recurse`
- `--verbose` -> `-Verbose`

## Functions

### `trim(const std::string& str)`

- Purpose: Remove leading and trailing spaces from a string.
- Behavior: Scans from both ends, trims spaces, and returns the cleaned substring.
- Used by: `translate()` to normalize command fragments before tokenization.

### `tokenize(const std::string& input)`

- Purpose: Split a string into tokens separated by spaces.
- Behavior: Iterates over characters, builds tokens, and pushes each completed token into a vector.
- Notes: Multiple spaces are handled by skipping empty tokens.
- Used by: `translate()` to separate each command and its arguments.

### `splitByAnd(const std::string& input)`

- Purpose: Break a full input string into individual commands separated by `&&`.
- Behavior: Searches for the literal substring `&&`, slices the input into segments, and returns them as a vector.
- Used by: `translate()` to support command chaining.

### `translateOne(const std::string& command, const std::vector<std::string>& args)`

- Purpose: Translate a single command plus arguments from shell-style to PowerShell-style.
- Behavior:
  - Looks up the command in `commandMap`.
  - If found, replaces it with the PowerShell equivalent.
  - If not found, keeps the original command text.
  - Iterates through `args`:
    - If an arg begins with `-`, attempts to map it via `flagMap`.
    - If the flag is unknown, it keeps the original argument.
    - Otherwise, appends the argument unchanged.
- Result: Returns a translated command string ready for PowerShell.

### `translate(const std::string& input)`

- Purpose: Translate the full user input, including any chained commands.
- Behavior:
  1. Split the input by `&&` using `splitByAnd()`.
  2. Trim and tokenize each command segment.
  3. Translate the command and arguments with `translateOne()`.
  4. Join translated segments using `; ` so PowerShell executes them sequentially.
- Example: `ls && pwd` becomes `Get-ChildItem; Get-Location`.

### `getUserInput()`

- Purpose: Read a full line of input from standard input.
- Behavior:
  - Uses `std::getline()` to read the user's text.
  - If the input is empty, prints `No input`.
  - Returns the raw input string.
- Notes: This is the entry point for user interaction.

### `executeCommand(const std::string& command)`

- Purpose: Run the translated command in PowerShell.
- Behavior:
  - Builds a command string:
    `powershell -NoProfile -ExecutionPolicy Bypass -Command "<translated command>"`
  - Calls `system()` to execute the resulting PowerShell command.
- Notes: The program relies on PowerShell being available in the environment.

### `main()`

- Behavior:
  1. Read user input via `getUserInput()`.
  2. Translate the input with `translate()`.
  3. Execute the result via `executeCommand()`.
  4. Return `0`.

## Execution Flow

1. User types a line like `ls -a && pwd`.
2. `main()` calls `getUserInput()`.
3. `translate()` splits the input into segments: `"ls -a"` and `"pwd"`.
4. Each segment is trimmed and tokenized.
5. `translateOne()` maps `ls` to `Get-ChildItem` and `-a` to `-Force`.
6. The translated command becomes `Get-ChildItem -Force; Get-Location`.
7. `executeCommand()` executes the PowerShell command string.

## What can break

- Quoted arguments such as `echo "hello world"` are not handled correctly; the tokenizer splits on every space and does not preserve quoted strings.
- Escaped spaces or special shell characters are not supported.
- `&&` is converted to `;` for PowerShell, so the second command runs even if the first command fails; this is not equivalent to shell `&&` semantics.
- Complex command syntax like pipes (`|`), redirection (`>`, `<`), subshells, or command grouping are not supported and will likely fail.
- Unknown flags are preserved, but unknown commands are not translated; if the unknown command is not valid in PowerShell, execution will fail.

## Notes

- The translation is basic and does not handle quoted arguments, escaped spaces, or advanced shell syntax.
- `&&` is converted to PowerShell semicolon execution; this means commands are run sequentially, but not conditionally on prior success.
- Unknown commands and flags are left unchanged so the program can still attempt execution.
