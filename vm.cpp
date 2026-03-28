//
// Created by marek on 11/24/2025.
//
#include "vm.h"

#include <iostream>
#include <variant>

void VM::advanceIP() {
    ++m_ip;
}

void VM::run() {
    const auto& code = m_program.code;

    while (m_ip < code.size()) {

        switch (const Instruction& ins = code[m_ip]; ins.op) {
            case OpCode::PUSH_INT: {
                m_stack.push((ins.argInt));
                advanceIP();
                break;
            }
            case OpCode::PUSH_FLOAT: {
                const auto idx = ins.argIndex;
                if (idx < 0 || static_cast<std::size_t>(idx) >= m_program.constants.size()) {
                    throw std::runtime_error("[VM]: Invalid constant index for PUSH_FLOAT");
                }

                const StackValue& v = m_program.constants[static_cast<std::size_t>(idx)];
                m_stack.push(v);   // pushes the real double
                advanceIP();
                break;
            }
            case OpCode::PUSH_BOOL: {
                m_stack.push(ins.argInt != 0);
                advanceIP();
                break;
            }
            case OpCode::PUSH_STRING: {
                const auto idx = ins.argIndex;
                if (idx < 0 || static_cast<std::size_t>(idx) >= m_program.constants.size()) {
                    throw std::runtime_error("[VM]: Invalid constant index for PUSH_STRING");
                }

                const StackValue& v = m_program.constants[static_cast<std::size_t>(idx)];
                m_stack.push(v);
                advanceIP();
                break;
            }
            // -------------------- Maths -------------------------
            case OpCode::ADD: {
                // TODO: Implement strings
                const StackValue r_value = m_stack.pop(); // TOP
                const StackValue l_value = m_stack.pop(); // Below TOP

                if (Utils::isNumber(l_value) && Utils::isNumber(r_value)) {
                    if (Utils::bothInt(l_value, r_value)) {
                        const int ia = std::get<int>(l_value);
                        const int ib = std::get<int>(r_value);
                        m_stack.push(ia + ib);
                    }
                    else {
                        const double la = Utils::toDouble(l_value);
                        const double ra = Utils::toDouble(r_value);

                        m_stack.push(la + ra);
                    }
                }
                else {
                    throw std::runtime_error("[VM]: Mismatched types for 'ADD' operation");
                }

                advanceIP();
                break;
            }
            case OpCode::SUB: {
                StackValue r_value = m_stack.pop(); // TOP
                StackValue l_value = m_stack.pop(); // Below TOP

                if (Utils::isNumber(l_value) && Utils::isNumber(r_value)) {
                    if (Utils::bothInt(l_value, r_value)) {
                        const int ia = std::get<int>(l_value);
                        const int ib = std::get<int>(r_value);
                        m_stack.push(ia - ib);
                    }
                    else {
                        const double la = Utils::toDouble(l_value);
                        const double ra = Utils::toDouble(r_value);

                        m_stack.push(la - ra);
                    }
                }
                else {
                    throw std::runtime_error("[VM]: Mismatched types for 'SUB' operation");
                }

                advanceIP();
                break;
            }
            case OpCode::MUL: {
                StackValue r_value = m_stack.pop(); // TOP
                StackValue l_value = m_stack.pop(); // Below TOP

                if (Utils::isNumber(l_value) && Utils::isNumber(r_value)) {
                    if (Utils::bothInt(l_value, r_value)) {
                        const int ia = std::get<int>(l_value);
                        const int ib = std::get<int>(r_value);

                        m_stack.push(ia * ib);
                    }
                    else {
                        const double la = Utils::toDouble(l_value);
                        const double ra = Utils::toDouble(r_value);

                        m_stack.push(la * ra);
                    }
                }
                else {
                    throw std::runtime_error("[VM]: Mismatched types for 'MUL' operation");
                }

                advanceIP();
                break;
            }
            case OpCode::DIV: {
                StackValue r_value = m_stack.pop();
                StackValue l_value = m_stack.pop();

                // Division by 0
                if (const double denom = Utils::toDouble(r_value); denom == 0.0) {
                    throw std::runtime_error("[VM]: Division by zero");
                }

                if (Utils::isNumber(l_value) && Utils::isNumber(r_value)) {
                    if (Utils::bothInt(l_value, r_value)) {
                        const int ia = std::get<int>(l_value);
                        const int ib = std::get<int>(r_value);

                        m_stack.push(ia / ib);
                    }
                    else {
                        const double la = Utils::toDouble(l_value);
                        const double ra = Utils::toDouble(r_value);

                        m_stack.push(la / ra);
                    }
                }
                else {
                    throw std::runtime_error("[VM]: Mismatched types for 'DIV' operation");
                }

                advanceIP();
                break;
            }
            case OpCode::MOD: {
                // TODO: this assumes only int
                StackValue r_value = m_stack.pop();
                StackValue l_value = m_stack.pop();

                // Mod by 0
                if (const double denom = Utils::toDouble(r_value); denom == 0.0) {
                    throw std::runtime_error("[VM]: Division by zero");
                }

                if (Utils::isNumber(l_value) && Utils::isNumber(r_value)) {
                    if (Utils::bothInt(l_value, r_value)) {
                        const int ia = std::get<int>(l_value);
                        const int ib = std::get<int>(r_value);

                         m_stack.push(ia % ib);
                    }
                }
                else {
                    throw std::runtime_error("[VM]: Mismatched types for 'MOD' operation");
                }

                advanceIP();
                break;
            }
            // --------------------- PRINT ---------------------
            case OpCode::PRINT: {
                if (m_stack.empty()) {
                    throw std::runtime_error("[VM]: Stack underflow on PRINT");
                }

                const auto v = m_stack.pop();
                std::cout << Utils::toDisplayString(v);
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
