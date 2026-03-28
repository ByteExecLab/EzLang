//
// Created by marek on 5/16/2025.
//

#include "Memory.h"

#include <iostream>
#include <bits/ostream.tcc>

Memory::Memory(const size_t size) {
    m_memory.resize(size, 0);
}

void Memory::write(const uint32_t address, const StackValue &value) {
    const std::size_t index = address;

    if (index >= m_memory.size()) {
        throw std::runtime_error("[ERROR]: Memory address out of bounds");
    }

    m_memory[index] = value;
}

StackValue Memory::read(const uint32_t address) const {
    const std::size_t index = address;

    if (index >= m_memory.size()) {
        throw std::runtime_error("[ERROR]: Memory address out of bounds");
    }

    return m_memory[index];
}

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


