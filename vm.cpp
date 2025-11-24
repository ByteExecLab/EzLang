//
// Created by marek on 11/24/2025.
//
#include "vm.h"

#include <iostream>
#include <variant>

void VM::run() {
    const auto& code = m_program.code;

    while (m_ip < code.size()) {

        switch (const Instruction& ins = code[m_ip]; ins.op) {
            case OpCode::PUSH_INT: {
                m_stack.push(static_cast<int>(ins.argInt));
                ++m_ip;
                break;
            }
            case OpCode::PUSH_FLOAT: {
                m_stack.push(static_cast<double>(ins.argInt));
                ++m_ip;
                break;
            }
            case OpCode::PUSH_BOOL: {
                m_stack.push(ins.argInt != 0);
                ++m_ip;
                break;
            }
            case OpCode::PUSH_STRING: {
                const auto idx = ins.argIndex;
                if (idx < 0 || static_cast<std::size_t>(idx) >= m_program.constants.size()) {
                    throw std::runtime_error("[VM]: Invalid constant index for PUSH_STRING");
                }

                const StackValue& v = m_program.constants[static_cast<std::size_t>(idx)];
                m_stack.push(v);
                ++m_ip;
                break;
            }
            // -------------------- Maths -------------------------
            case OpCode::ADD: {
                // TODO: this assumes both are INTs for now
                auto a = m_stack.pop();
                auto b = m_stack.pop();

                const int ia = std::get<int>(a);
                const int ib = std::get<int>(b);

                m_stack.push(ib + ia);
                ++m_ip;
                break;
            }
            case OpCode::SUB: {
                // TODO: this assumes both are INTs for now
                auto a = m_stack.pop();
                auto b = m_stack.pop();

                const int ia = std::get<int>(a);
                const int ib = std::get<int>(b);

                m_stack.push(ib - ia);
                ++m_ip;
                break;
            }
            case OpCode::MUL: {
                // TODO: this assumes only int
                auto a = m_stack.pop();
                auto b = m_stack.pop();

                const int ia = std::get<int>(a);
                const int ib = std::get<int>(b);

                m_stack.push(ib * ia);
                ++m_ip;
                break;
            }
            case OpCode::DIV: {
                // TODO: this assumes only int
                auto a = m_stack.pop();
                auto b = m_stack.pop();

                const int ia = std::get<int>(a);
                const int ib = std::get<int>(b);

                m_stack.push(ib / ia);
                ++m_ip;
                break;
            }
            case OpCode::MOD: {
                // TODO: this assumes only int
                auto a = m_stack.pop();
                auto b = m_stack.pop();

                const int ia = std::get<int>(a);
                const int ib = std::get<int>(b);

                m_stack.push(ib % ia);
                ++m_ip;
                break;
            }
            // --------------------- PRINT ---------------------
            case OpCode::PRINT: {
                if (m_stack.empty()) {
                    throw std::runtime_error("[VM]: Stack underflow on PRINT");
                }

                const auto v = m_stack.pop();
                std::visit([]<typename T0>(const T0& x) {
                    using T = std::decay_t<T0>;
                    if constexpr (std::is_same_v<T, std::monostate>) {
                        std::cout << "nil";
                    } else if constexpr (std::is_same_v<T, ArrayValue>) {
                        std::cout << "<Array len=" << x.elements.size() << ">";
                    } else if constexpr (std::is_same_v<T, StructValue>) {
                        std::cout << "<Struct fields=" << x.fields.size() << ">";
                    } else if constexpr (std::is_same_v<T, bool>) {
                        std::cout << (x ? "true" : "false");
                    } else {
                        // int, double, std::string all have operator<<
                        std::cout << x;
                    }
                }, v);
                std::cout << "\n";
                ++m_ip;
                break;
            }
            case OpCode::HLT: {
                return;
            }
            default: {
                throw std::runtime_error(
                    std::string("[VM]: Unimplemented opcode: ") + toString(ins.op)
                );
            }
        }
    }
}
