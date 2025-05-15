//
// Created by marek on 5/14/2025.
//

#ifndef STACK_H
#define STACK_H
#include <stack>
#include <string>
#include <variant>


class Stack {
private:
    std::stack<std::variant<int, std::string>> m_stack;

public:
    // Check if the stack is empty
    [[nodiscard]]
    bool empty() const;

    // Push onto the stack
    void push(const std::variant<int, std::string>& value);

    // Duplicate
    void dup();

    // Pop the value from the stack
    std::variant<int, std::string> pop();

    // Peek the value on top of the stack
    std::variant<int, std::string> peek();

    // Swap the values on top of the stack
    void swap();

    // Drop the value from the stack
    void drop();

    // Get stack size
    [[nodiscard]]
    size_t size() const;
};



#endif //STACK_H
