#include <iostream>
#include <vector>
#include <string>

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
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) std::cout << "No input\n";
    return input;
}

// Execute the translated command in PowerShell.
// Uses a minimal profile to avoid user-specific startup scripts.
void executeCommand(const std::string& command) {
    std::string psCommand =
        "powershell -NoProfile -ExecutionPolicy Bypass -Command \"" + command + "\"";
    system(psCommand.c_str());
}

int main() {
    std::cout << "psFlip> " << std::flush;
    std::string input  = getUserInput();
    std::string result = translate(input);
    executeCommand(result);
    return 0;
}