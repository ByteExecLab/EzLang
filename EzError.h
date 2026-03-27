//
// Created by marek on 3/27/2026.
//

#ifndef EZLANG_EZERROR_H
#define EZLANG_EZERROR_H
#include <stdexcept>
#include <string>

enum class EzErrorPhase {
    Load,
    Tokenize,
    Parse,
    Runtime
};

struct EzSourceLocation {
    std::string module = "<unknown>";
    size_t line = 0;
    size_t column = 0;
};

struct EzError {
    EzErrorPhase phase;
    std::string message;
    EzSourceLocation location;
    std::string snippet;
};

class EzException : public std::runtime_error {
public:
    explicit EzException(EzError error) : std::runtime_error(error.message), m_error(std::move(error)){}
    [[nodiscard]] EzError error() const {
        return m_error;
    }
private:
    EzError m_error;
};

#endif //EZLANG_EZERROR_H