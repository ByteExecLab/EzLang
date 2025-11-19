#ifndef STACK_H
#define STACK_H
#include <stack>
#include <string>
#include <variant>
#include <vector>
#include "Common.h"

class Stack {
private:
    std::stack<std::variant<int, double, std::string, bool>> m_stack;

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
    void push(const StackValue& value);

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
    StackValue pop();

    /**
     * Returns the top value of the stack without removing it.
     *
     * @return The top value of the stack, which can be an integer or a string.
     * @throws std::runtime_error If the stack is empty.
     */
    [[nodiscard]]
    StackValue peek() const;

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

    [[nodiscard]]
    std::vector<StackValue> getContents() const;

    /**
     * Copies the second element from the top of the stack and pushes it onto the stack.
     *
     * The method duplicates the second element on the stack and places it on top.
     *
     * @throws std::runtime_error If the stack contains fewer than two elements.
     */
    void over();

    /**
     * Moves the top element of the stack to the second position, duplicating it.
     *
     * The method takes the top element of the stack, temporarily removes the top two elements,
     * pushes the original top element back onto the stack, followed by the second element,
     * and then pushes the original top element again.
     *
     * @throws std::runtime_error If the stack contains fewer than two elements.
     */
    void tuck();

    /**
     * Removes all elements from the stack.
     *
     * After calling this method, the stack will be empty.
     */
    void clear();

    /**
     * Returns the number of elements in the stack.
     *
     * @return The number of elements currently in the stack.
     */
    [[nodiscard]]
    size_t size() const;
};



#endif //STACK_H
