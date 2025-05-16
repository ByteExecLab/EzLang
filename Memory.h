//
// Created by marek on 5/16/2025.
//

#ifndef MEMORY_H
#define MEMORY_H
#include <cstdint>
#include <string>
#include <variant>
#include <variant>
#include <vector>

using StackValue = std::variant<int, std::string>;

class Memory {
public:
    explicit Memory(size_t size);

    void write(uint32_t address, const StackValue& value);

    StackValue read(uint32_t address) const;

    void dump() const;
    size_t size() const;
private:
    std::vector<StackValue> m_memory;
    size_t m_size = 0;
};

#endif //MEMORY_H
