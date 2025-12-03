//
// Created by marek on 12/3/2025.
//

#ifndef EZLANG_PLATFORM_IO_H
#define EZLANG_PLATFORM_IO_H
#include <optional>
#include <string>

namespace platform_io {
    /**
     * Reads a line of text from the standard input stream.
     *
     * This function attempts to read a single line of input from the user.
     * If input is successfully read, it returns an optional containing the
     * line as a string. If no input is available (e.g., end-of-file is reached),
     * it returns an empty optional.
     *
     * @return An optional string containing the line of input if successful,
     * or an empty optional if no input is available.
     */
    std::optional<std::string> read_line();

    /**
     * Writes a string to the standard output stream.
     *
     * This function outputs the given string to the standard output.
     * It sends the string content exactly as provided, allowing the user
     * to display messages or data to the console or log.
     *
     * @param s The string to be written to the standard output.
     */
    void write_string(const std::string& s);
}

#endif //EZLANG_PLATFORM_IO_H