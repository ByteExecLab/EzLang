//
// Created by marek on 5/16/2025.
//

#include "Memory.h"

#include <iostream>
#include <bits/ostream.tcc>

/**
 * Constructs a Memory object with the specified size, initializing all memory locations to zero.
 *
 * @param size The number of memory locations to allocate and initialize.
 * @return None
 */
Memory::Memory(const size_t size) {
    m_memory.resize(size, 0);
}

/**
 * Writes a value to the specified memory address.
 *
 * @param address The memory address where the value should be written.
 *                Must be within the bounds of the allocated memory.
 * @param value The value to write to the specified memory address.
 *              Can contain different types as defined by the StackValue variant.
 * @throw std::runtime_error If the specified address is out of bounds.
 */
void Memory::write(uint32_t address, const StackValue &value) {
    if (address >= m_memory.size()) {
        throw std::runtime_error("[ERROR]: Memory address out of bounds");
    }

    m_memory[address] = value;
}

/**
 * Reads the value stored at the specified memory address.
 *
 * @param address The memory address from which the value is to be read.
 * @return The value stored at the specified memory address.
 * @throws std::runtime_error If the provided address is out of bounds.
 */
StackValue Memory::read(const uint32_t address) const {
    if (address >= m_memory.size()) {
        throw std::runtime_error("[ERROR]: Memory address out of bounds");
    }

    return m_memory[address];
}

/**
 * Outputs the current state of the memory to the standard output.
 *
 * This method iterates over all memory locations and prints each index
 * followed by its value. If the value is an integer, it prints the integer.
 * If the value is a string, it prints the string.
 *
 * @return None
 */
void Memory::dump() const {
    std::cout << "Memory dump:" << std::endl;
    for (size_t i = 0; i < m_size; i++) {
        if (const StackValue value = m_memory[i]; std::holds_alternative<int>(value)) {
            std::cout << "[" << i << "] = " << std::get<int>(value) << std::endl;
        }
        else {
            std::cout << "[" << i << "] = " << std::get<std::string>(value) << std::endl;
        }
    }
    std::cout << std::endl;
}


