//
// Created by marek on 5/14/2025.
//

#include "Stack.h"

#include <stdexcept>
#include <variant>

// Push the value onto the stack
void Stack::push(const std::variant<int, std::string>& value) {
    m_stack.push(value);
}

// Duplicates the values on top of the stack
void Stack::dup() {
    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack is empty");
    }

    // top: does leave an element on the stack
    const auto value = m_stack.top();
    m_stack.push(value);
}

// Removes the top of the stack and returns
std::variant<int, std::string> Stack::pop() {
    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack is empty");
    }

    auto value = m_stack.top();
    m_stack.pop();
    return value;
}

// Returns the value on top of the stack
std::variant<int, std::string> Stack::peek() {
    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack is empty");
    }

    return m_stack.top();
}

// Swaps the values on top of the stack
void Stack::swap() {
    if (m_stack.size() < 2) {
        throw std::runtime_error("[ERROR]: Stack is empty");
    }

    const auto value1 = m_stack.top();
    m_stack.pop();
    const auto value2 = m_stack.top();
    m_stack.pop();

    m_stack.push(value1);
    m_stack.push(value2);
}

// Removes the top of the stack
void Stack::drop() {
    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack is empty");
    }

    m_stack.pop();
}

// Is stack empty?
bool Stack::empty() const {
    return m_stack.empty();
}

// Get the size of the stack
size_t Stack::size() const {
    return m_stack.size();
}

