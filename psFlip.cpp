#include <iostream>
#include <vector>
#include <string>
#include <conio.h>
#include <windows.h>
#include <fstream>

// Map common Linux-like commands to equivalent PowerShell commands.
// This lets users type familiar commands while the tool runs the translated version.
const char* commandMap[][2] = {
    {"ls",       "Get-ChildItem"},
    {"pwd",      "Get-Location"},
    {"cd",       "Set-Location"},
    {"cat",      "Get-Content"},
    {"touch",    "New-Item"},
    {"mkdir",    "New-Item -ItemType Directory"},
    {"rm",       "Remove-Item"},
    {"cp",       "Copy-Item"},
    {"mv",       "Move-Item"},
    {"find",     "Get-ChildItem"},
    {"grep",     "Select-String"},
    {"which",    "Get-Command"},
    {"echo",     "Write-Output"},
    {"clear",    "Clear-Host"},
    {"ps",       "Get-Process"},
    {"kill",     "Stop-Process"},
    {"curl",     "Invoke-WebRequest"},
    {"wget",     "Invoke-WebRequest"},
    {"tar",      "tar"},
    {"git",      "git"},
    {"npm",      "npm"},
    {"node",     "node"},
    {"python3",   "python"},
    {"pip3",      "pip"},
    {"docker",   "docker"}
};

// Convert Linux-style flags into PowerShell flag equivalents when possible.
const char* flagMap[][2] = {
    {"-a",           "-Force"},
    {"-r",           "-Recurse"},
    {"-f",           "-Force"},
    {"-rf",          "-Recurse -Force"},
    {"-fr",          "-Force -Recurse"},
    {"-i",           "-Confirm"},
    {"-v",           "-Verbose"},
    {"--help",       "-?"},
    {"--force",      "-Force"},
    {"--recursive",  "-Recurse"},
    {"--verbose",    "-Verbose"}
};


// Remove leading and trailing spaces from a command segment.
// This normalizes user input before tokenization.
std::string trim(const std::string& str) {
    int start = 0;
    int end = (int)str.size() - 1;

    while (start <= end && str[start] == ' ') start++;
    while (end >= start && str[end]   == ' ') end--;

    if (start > end) return "";
    return str.substr(start, end - start + 1);
}


// Split a command string into words based on spaces.
// The first token is the command; the rest are its arguments.
std::vector<std::string> tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;

    for (char c : input) {
        if (c == ' ') {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) tokens.push_back(token);

    return tokens;
}


// Break the input into separate commands at '&&'.
// This allows multiple commands to be translated and executed in sequence.
std::vector<std::string> splitByAnd(const std::string& input) {
    std::vector<std::string> commands;
    size_t start = 0;
    size_t pos   = 0;

    while ((pos = input.find("&&", start)) != std::string::npos) {
        commands.push_back(input.substr(start, pos - start));
        start = pos + 2;
    }
    commands.push_back(input.substr(start));

    return commands;
}


// Translate a single command and its arguments.
// Commands are converted using commandMap, and flags are converted using flagMap.
std::string translateOne(const std::string& command, const std::vector<std::string>& args) {
    std::string result = command;

    int cmdSize = sizeof(commandMap) / sizeof(commandMap[0]);
    for (int i = 0; i < cmdSize; i++) {
        if (command == commandMap[i][0]) {
            result = commandMap[i][1];
            break;
        }
    }

    int flagSize = sizeof(flagMap) / sizeof(flagMap[0]);
    for (const std::string& arg : args) {
        if (!arg.empty() && arg[0] == '-') {
            bool found = false;
            for (int i = 0; i < flagSize; i++) {
                if (arg == flagMap[i][0]) {
                    result += " " + std::string(flagMap[i][1]);
                    found = true;
                    break;
                }
            }
            
            if (!found) result += " " + arg;
        } else {
            result += " " + arg;
        }
    }

    return result;
}


// Translate a full user input line, handling chained commands.
std::string translate(const std::string& input) {
    std::vector<std::string> segments = splitByAnd(input);
    std::string result;

    for (int i = 0; i < (int)segments.size(); i++) {
        std::string seg = trim(segments[i]);
        if (seg.empty()) continue;

        std::vector<std::string> tokens = tokenize(seg);
        if (tokens.empty()) continue;

        std::string command = tokens[0];
        std::vector<std::string> args(tokens.begin() + 1, tokens.end());

        if (!result.empty()) result += "; ";
        result += translateOne(command, args);
    }

    return result;
}

// Read a full line from standard input.
// If the user enters nothing, print a warning and return an empty string.
std::string getUserInput() {
    // Simple line editor that supports Tab completion and basic editing.
    std::string line;
    const char PROMPT[] = "psFlip> ";

    while (true) {
        int ch = _getch();
        if (ch == 13) { // Enter
            std::cout << std::endl;
            break;
        } else if (ch == 8) { // Backspace
            if (!line.empty()) {
                line.pop_back();
                std::cout << "\b \b" << std::flush;
            }
        } else if (ch == 9) { // Tab
            // Find last token prefix
            size_t pos = line.find_last_of(' ');
            std::string prefix = (pos == std::string::npos) ? line : line.substr(pos + 1);

            std::vector<std::string> candidates;
            int cmdSize = sizeof(commandMap) / sizeof(commandMap[0]);
            for (int i = 0; i < cmdSize; ++i) {
                std::string cmd = commandMap[i][0];
                if (cmd.rfind(prefix, 0) == 0) candidates.push_back(cmd);
            }

            // enumerate files in current directory using WinAPI for better portability
            std::string pattern = prefix + "*";
            WIN32_FIND_DATAA findData;
            HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    std::string name = findData.cFileName;
                    if (name.rfind(prefix, 0) == 0) candidates.push_back(name);
                } while (FindNextFileA(hFind, &findData));
                FindClose(hFind);
            }

            if (candidates.size() == 1) {
                std::string completion = candidates[0].substr(prefix.size());
                line += completion;
                std::cout << completion << std::flush;
            } else if (candidates.size() > 1) {
                std::cout << std::endl;
                for (auto &c : candidates) std::cout << c << "  ";
                std::cout << std::endl;
                std::cout << PROMPT << line << std::flush;
            }
        } else if (ch == 3) { // Ctrl-C
            std::cout << "^C" << std::endl;
            line.clear();
            break;
        } else {
            line.push_back((char)ch);
            std::cout << (char)ch << std::flush;
        }
    }

    if (line.empty()) std::cout << "No input\n";
    return line;
}


// Detect a bash-style for loop and translate to PowerShell foreach.
// Example supported pattern: for i in 1 2 3; do echo $i; done
std::string translateForLoop(const std::string &input) {
    std::string s = input;
    // naive detection
    if (s.rfind("for ", 0) != 0) return "";

    size_t inPos = s.find(" in ");
    size_t doPos = s.find("; do ");
    size_t donePos = s.rfind("; done");
    if (inPos == std::string::npos || doPos == std::string::npos || donePos == std::string::npos) return "";

    std::string var = trim(s.substr(4, inPos - 4));
    std::string list = trim(s.substr(inPos + 4, doPos - (inPos + 4)));
    std::string body = trim(s.substr(doPos + 5, donePos - (doPos + 5)));

    if (var.empty() || list.empty() || body.empty()) return "";

    // convert list (space separated) into PowerShell comma list
    std::vector<std::string> items = tokenize(list);
    std::string psList;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) psList += ",";
        psList += items[i];
    }

    std::string inner = translate(body);
    if (inner.empty()) inner = body;

    std::string result = "foreach ($" + var + " in " + psList + ") { " + inner + " }";
    return result;
}

// Execute the translated command in PowerShell.
// Uses a minimal profile to avoid user-specific startup scripts.
void executeCommand(const std::string& command) {
    std::string psCommand =
        "powershell -NoProfile -ExecutionPolicy Bypass -Command \"" + command + "\"";
    system(psCommand.c_str());
}

int main() {
    // support command-line modes for automated tests
    int argcV = __argc;
    char** argvV = __argv;
    bool noExec = false;
    int modeIndex = -1; // index where -c or -f appears
    std::string mode;
    for (int i = 1; i < argcV; ++i) {
        std::string a = argvV[i];
        if (a == "-n") {
            noExec = true;
        } else if (a == "-c" || a == "-f") {
            mode = a;
            modeIndex = i;
            break;
        }
    }

    if (modeIndex != -1) {
        if (mode == "-c") {
            // join remaining args as a single command
            std::string cmd;
            for (int i = modeIndex + 1; i < argcV; ++i) {
                if (i > modeIndex + 1) cmd += " ";
                cmd += argvV[i];
            }
            std::string result = translateForLoop(cmd);
            if (result.empty()) result = translate(cmd);
            if (!result.empty()) {
                if (noExec) std::cout << result << std::endl;
                else executeCommand(result);
            }
            return 0;
        } else if (mode == "-f") {
            if (modeIndex + 1 < argcV) {
                std::string path = argvV[modeIndex + 1];
                std::ifstream in(path);
                if (in) {
                    std::string cmd((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                    cmd = trim(cmd);
                    std::string result = translateForLoop(cmd);
                    if (result.empty()) result = translate(cmd);
                    if (!result.empty()) {
                        if (noExec) std::cout << result << std::endl;
                        else executeCommand(result);
                    }
                } else {
                    std::cerr << "Failed to open file: " << path << std::endl;
                }
            } else {
                std::cerr << "Missing file path after -f" << std::endl;
            }
            return 0;
        }
    }

    while (true) {
        std::cout << "psFlip> " << std::flush;
        std::string input = getUserInput();
        if (input == "" ) continue;
        if (input == "exit" || input == "quit") break;

        std::string result = translateForLoop(input);
        if (result.empty()) result = translate(input);

        if (result.empty()) continue;
        executeCommand(result);
    }

    return 0;
}