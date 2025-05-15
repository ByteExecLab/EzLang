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
    /**
     * Checks if the stack is empty.
     *
     * @return True if the stack is empty, otherwise false.
     */
    [[nodiscard]]
    bool empty() const;

    /**
     * Pushes a value onto the stack.
     *
     * @param value The value to push onto the stack. This can be an integer or a string.
     */
    void push(const std::variant<int, std::string>& value);

    /**
     * Duplicates the top element of the stack and pushes it onto the stack.
     *
     * @throws std::runtime_error If the stack is empty.
     */
    void dup();

    /**
     * Removes and returns the top value from the stack.
     *
     * @return The top value of the stack, which can be an integer or a string.
     * @throws std::runtime_error If the stack is empty.
     */
    std::variant<int, std::string> pop();

    /**
     * Returns the top value of the stack without removing it.
     *
     * @return The top value of the stack, which can be an integer or a string.
     * @throws std::runtime_error If the stack is empty.
     */
    std::variant<int, std::string> peek();

    /**
     * Swaps the top two elements of the stack.
     *
     * The method exchanges the positions of the two elements on top of the stack.
     *
     * @throws std::runtime_error If the stack contains fewer than two elements.
     */
    void swap();

    /**
     * Removes the top element from the stack.
     *
     * If the stack is empty, an exception is thrown.
     *
     * @throws std::runtime_error If the stack is empty.
     */
    void drop();

    /**
     * Returns the number of elements in the stack.
     *
     * @return The number of elements currently in the stack.
     */
    [[nodiscard]]
    size_t size() const;
};



#endif //STACK_H
