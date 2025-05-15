//
// Created by marek on 5/14/2025.
//

#include "Stack.h"

#include <iostream>
#include <ostream>
#include <stdexcept>
#include <variant>

/**
 * Pushes a value onto the stack.
 *
 * @param value The value to be pushed onto the stack. It can be either an integer or a string.
 */
void Stack::push(const std::variant<int, std::string>& value) {
    m_stack.push(value);
}

/**
 * Duplicates the top element of the stack and pushes it onto the stack.
 *
 * @throws std::runtime_error If the stack is empty.
 */
void Stack::dup() {
    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack is empty");
    }

    // top: does leave an element on the stack
    const auto value = m_stack.top();
    m_stack.push(value);
}

/**
 * Removes and returns the top value from the stack.
 *
 * @return The top value of the stack, which can be an integer or a string.
 * @throws std::runtime_error If the stack is empty.
 */
std::variant<int, std::string> Stack::pop() {
    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack is empty");
    }

    auto value = m_stack.top();
    m_stack.pop();
    return value;
}

/**
 * Returns the top value of the stack without removing it.
 *
 * @return The top value of the stack, which can be an integer or a string.
 * @throws std::runtime_error If the stack is empty.
 */
std::variant<int, std::string> Stack::peek() {
    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack is empty");
    }

    return m_stack.top();
}

/**
 * Swaps the top two elements of the stack.
 *
 * This method exchanges the positions of the two elements
 * at the top of the stack without modifying their values.
 *
 * @throws std::runtime_error If the stack contains fewer than two elements.
 */
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

/**
 * Removes the top element from the stack.
 *
 * If the stack is empty, an exception is thrown to indicate the error.
 *
 * @throws std::runtime_error If the stack is empty.
 */
void Stack::drop() {
    if (m_stack.empty()) {
        throw std::runtime_error("[ERROR]: Stack is empty");
    }

    m_stack.pop();
}

/**
 * Checks if the stack is empty.
 *
 * @return True if the stack has no elements, otherwise false.
 */
bool Stack::empty() const {
    return m_stack.empty();
}

/**
 * Returns the number of elements in the stack.
 *
 * @return The number of elements currently stored in the stack.
 */
size_t Stack::size() const {
    return m_stack.size();
}

