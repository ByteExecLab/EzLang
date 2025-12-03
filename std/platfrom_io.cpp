//
// Created by marek on 12/3/2025.
//
#include "platform_io.h"

#ifdef _WIN32
#ifndef NOMINMAX
    #define NOMINMAX
#endif
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#include <vector>

namespace platform_io {

#ifdef _WIN32
    std::optional<std::string> read_line() {
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        if (hIn == INVALID_HANDLE_VALUE) {
            return std::nullopt;
        }

        std::string line;
        char ch;
        DWORD bytesRead = 0;

        while (true) {
            if (!ReadFile(hIn, &ch, 1, &bytesRead, nullptr) || bytesRead == 0) {
                // EOF or error
                if (line.empty()) {
                    return std::nullopt;
                    break;
                }
            }

            if (ch == '\r') {
                // maybe followed by '\n', swallow if present
                char next;
                DWORD br2 = 0;
                if (ReadFile(hIn, &next, 1, &br2, nullptr) && br2 == 1 && next != '\n') {
                    line.push_back(next);
                }
                break;
            }

            if (ch == '\n') {
                break;
            }

            line.push_back(ch);
        }

        return line;
    }

    void write_string(const std::string& s) {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;

        DWORD written = 0;
        // Safe because std::string is contiguous
        WriteFile(hOut, s.data(), static_cast<DWORD>(s.size()), &written, nullptr);
    }

#else // POSIX-ish (Linux, macOS, etc.)

    std::optional<std::string> read_line() {
        std::string line;
        char ch;
        ssize_t n;

        while (true) {
            n = ::read(STDIN_FILENO, &ch, 1);
            if (n == 0) {
                // EOF
                if (line.empty()) return std::nullopt;
                break;
            }
            if (n < 0) {
                // error
                return std::nullopt;
            }

            if (ch == '\n') break;
            line.push_back(ch);
        }

        return line;
    }


    void write_string(const std::string& s) {
        const char* data = s.data();
        size_t total = s.size();

        while (total > 0) {
            ssize_t written = ::write(STDOUT_FILENO, data, total);
            if (written <= 0) {
                // error, just bail
                break;
            }
            data  += written;
            total -= written;
        }
    }

#endif

}