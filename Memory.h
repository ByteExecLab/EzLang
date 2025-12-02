//
// Created by marek on 5/16/2025.
//

#ifndef MEMORY_H
#define MEMORY_H
#include <string>
#include <variant>
#include <vector>
#include "Common.h"

class Memory {
public:
    explicit Memory(size_t size);

    /**
     * Writes a value into the memory at a specified address.
     *
     * This method calculates the memory index based on the given address and assigns
     * the provided `value` to the corresponding position in the memory array. If the
     * calculated index is out of the bounds of the memory, an exception will be thrown.
     *
     * @param address The memory address where the value should be written. This value
     *                is divided by the size of `StackValue` to determine the actual
     *                index in the memory array.
     * @param value The `StackValue` to be written into the memory at the specified address.
     *              The `StackValue` can hold multiple types such as int, double, string, etc.
     *
     * @throws std::runtime_error If the calculated memory index exceeds the bounds of
     *                            the memory array.
     */
    void write(uint32_t address, const StackValue& value);

    /**
     * Reads a value from the memory at a specified address.
     *
     * This method calculates the memory index based on the given address and retrieves
     * the `StackValue` stored at that index in the memory array. If the calculated index
     * is out of the bounds of the memory, an exception will be thrown.
     *
     * @param address The memory address from which the value should be read. This value
     *                is divided by the size of `StackValue` to determine the actual
     *                index in the memory array.
     *
     * @return The `StackValue` retrieved from the specified memory address. The `StackValue`
     *         may contain types such as int, double, string, bool, array, or struct.
     *
     * @throws std::runtime_error If the calculated memory index exceeds the bounds of
     *                            the memory array.
     */
    [[nodiscard]]
    StackValue read(uint32_t address) const;

    /**
     * Outputs the current state of memory to the console for debugging purposes.
     *
     * This method iterates over the memory array and prints the index and value of
     * each memory cell. It handles different types stored in the memory, such as integers
     * and strings, and outputs them accordingly. The output format includes the memory
     * index and its value. This method is primarily used for debugging and examining
     * the content of the memory during program execution.
     */
    void dump() const;

private:
    /**
     * Represents the underlying memory storage for the `Memory` class.
     *
     * This vector is used to store a collection of `StackValue` objects, which can represent
     * different types such as integers, floating-point numbers, strings, booleans, arrays, or
     * structs. The size of the memory is defined during the initialization of the `Memory`
     * class and is fixed for the lifetime of the object.
     *
     * The elements in this vector are accessed by index, which is calculated based on
     * memory addresses provided by the user. The methods `write` and `read` ensure that
     * memory operations are performed safely, throwing exceptions if the specified address
     * is out of bounds.
     */
    std::vector<StackValue> m_memory;

    /**
     * Specifies the size of the memory in terms of the number of elements it can hold.
     *
     * This variable represents an integral count of how many `StackValue` objects
     * can be stored in the `m_memory` vector. It is initialized when the `Memory` class
     * is instantiated and remains constant throughout the object's lifetime. The value
     * of `m_size` determines the valid range for memory operations such as `read` and `write`.
     *
     * @note Accessing indices beyond the range defined by `m_size` will result in
     *       runtime exceptions to ensure safety of memory operations.
     */
    size_t m_size = 0;
};

#endif //MEMORY_H
