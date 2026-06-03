# psFlip

## The Problem

The IDE ai agents run Linux/bash and occasionally work on Windows, and waste lot of tokens for running the commands, you already know the pain :)

```bash
$ ls -la          # muscle memory kicks in
$ rm -rf dist     # typed it before thinking
$ grep "error" .  # classic
```

All wrong. PowerShell doesn't speak bash. So what do most people do?

1. Type the bash command out of habit
2. Get a red error
3. Ask an AI agent *"what's the PowerShell equivalent of `rm -rf`?"*
4. AI burns tokens looking it up, generating a response, explaining it
5. You paste the command
6. Repeat this **every single session**

That token loop is wasteful — especially inside agentic workflows where every LLM call has a cost. If your agent is spending compute just to translate `ls` → `Get-ChildItem` on every run, something is wrong.

---

## The Fix

**psFlip** is a lightweight, under 1 MB offline C++ CLI tool that sits between your bash muscle memory and PowerShell. You type bash, it outputs PowerShell — instantly, locally, zero API calls.

```bash
run> psFlip.exe
psFlip> cd /home/user/projects    →  Set-Location /home/user/projects
psFlip> grep "TODO" src/main.cpp  →  Select-String "TODO" src/main.cpp
psFlip> rm -rf dist               → Remove-Item -Recurse -Force dist
psFlip> mkdir new-folder          →  New-Item -ItemType Directory new-folder
```

No internet. No model. No token burn. Just a lookup table and a compiled binary.

---

## Build

No external libraries.
Just run the powershell script in Powershell Administrator mode and Press [Y] to edit the system PATH.
That's it.

```bash
# MinGW / GCC
.\compile.ps1
```
---

Thank you for using psFlip! If you find a command missing, feel free to contribute by adding it to the `commands.txt` file and recompiling. Happy coding!
