//
// Created by marek on 5/14/2025.
//

#include "Stack.h"

#include <iosfwd>
#include <iostream>
#include <stdexcept>
#include <variant>
#include <vector>
#include "Interpreter.h"

/**
 * Pushes a value onto the stack.
 *
 * @param value The value to be pushed onto the stack. It can be either an integer or a string.
 */
void Stack::push(const std::variant<int, double, std::string>& value) {
    m_stack.push(value);
}

/**
 * Duplicates the top element of the stack and pushes it onto the stack.
 *
 * @throws std::runtime_error If the stack is empty.
 */
void Stack::dup() {
    if (m_stack.empty()) {
        throw std::runtime_error("[Stack::dup]: Stack must have at least one element to dup");
    }

    // top: does leave an element on the stack
    const auto value = m_stack.top();
    m_stack.push(value);
}


std::vector<StackValue> Stack::getContents() const {
    std::vector<StackValue> values;
    std::stack<StackValue> temp = m_stack;

    while (!temp.empty()) {
        values.push_back(temp.top());
        temp.pop();
    }

    std::reverse(values.begin(), values.end());
    return values;
}


/**
 * Copies the second-to-top element of the stack and pushes it onto the stack.
 *
 * This method duplicates the second element from the top and places it on top
 * without altering the order of the remaining elements. The original top element
 * remains in its position after the operation.
 *
 * @throws std::runtime_error If the stack contains fewer than two elements.
 */
void Stack::over() {
    if (m_stack.size() < 2) {
        throw std::runtime_error("[Stack::over]: Stack must have at least 2 elements to over");
    }

    const auto top = m_stack.top(); m_stack.pop();
    const auto second = m_stack.top();
    m_stack.push(top);
    m_stack.push(second);
}

/**
 * Inserts the top value of the stack beneath the second value.
 *
 * The method rearranges the top two elements of the stack, so the top value is pushed beneath the second one.
 * The resulting order will have the top value duplicated and inserted between the original second element and itself.
 *
 * @throws std::runtime_error If the stack contains fewer than two elements.
 */
void Stack::tuck() {
    if (m_stack.size() < 2) {
        throw std::runtime_error("[Stack::tuck]: Stack must have at least 2 elements to tuck");
    }

    const auto top = m_stack.top(); m_stack.pop();
    const auto second = m_stack.top(); m_stack.pop();

    m_stack.push(top);
    m_stack.push(second);
    m_stack.push(top);
}

/**
 * Removes and returns the top value from the stack.
 *
 * @return The top value of the stack, which can be an integer or a string.
 * @throws std::runtime_error If the stack is empty.
 */
std::variant<int, double, std::string> Stack::pop() {
    if (m_stack.empty()) {
        throw std::runtime_error("[Stack::pop]: Stack must have at least one element to pop");
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
std::variant<int, double, std::string> Stack::peek() const {
    if (m_stack.empty()) {
        throw std::runtime_error("[Stack::peek]: Stack must have at least one element to peek");
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
        throw std::runtime_error("[Stack::swap]: Stack must have at least 2 elements to swap");
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
        throw std::runtime_error("[Stack::drop]: Stack must have at least one element to drop");
    }

    m_stack.pop();
}

/**
 * Removes all elements from the stack.
 *
 * This method clears the stack by removing each element until it is empty.
 */
void Stack::clear() {
    while (!m_stack.empty()) m_stack.pop();
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

