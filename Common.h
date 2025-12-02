//
// Created by marek on 5/18/2025.
//

#ifndef COMMON_H
#define COMMON_H
#include <memory>
#include <string>
#include <variant>
#include <unordered_map>
#include <vector>

struct ArrayValue;
struct StructValue;

using StackValue = std::variant<
    std::monostate,
    int,
    double,
    std::string,
    bool,
    ArrayValue,
    StructValue
>;

/**
 * Represents an array-like structure, holding a collection of elements.
 * Each element is a shared pointer to a `StackValue`.
 * The primary purpose of this structure is to manage and store a dynamic set
 * of interpreted values used within the program.
 */
struct ArrayValue {
    /**
     * @brief A container for storing elements used in the stack-based virtual machine.
     *
     * This variable holds a collection of shared pointers to `StackValue` objects,
     * which can represent various types of data (e.g., integers, strings, booleans,
     * arrays, structs, etc.). It is a critical part of the `ArrayValue` structure
     * and is used to manage the elements within an array. The use of shared pointers
     * allows for reference counting and shared ownership of the stored elements.
     */
    std::vector<std::shared_ptr<StackValue>> elements;

    /**
     * Default constructor for the ArrayValue class.
     *
     * @return A new instance of ArrayValue with default initialization.
     */
    ArrayValue() = default;

    /**
     * Constructs an ArrayValue object from a vector of StackValue objects.
     * Each element in the provided vector is moved into a shared pointer and stored in the `elements` vector of the ArrayValue.
     *
     * @param src A vector of StackValue objects to initialize the ArrayValue instance.
     *            These values are moved into shared pointers internally.
     * @return An instance of ArrayValue containing the provided StackValue objects.
     */
    explicit ArrayValue(std::vector<StackValue> src);
};

/**
 * Represents a structured collection of named values.
 * Each field in the struct is stored as a key-value pair, where the key is a string
 * representing the field name, and the value is a shared pointer to a `StackValue`.
 * This structure is used to manage and encapsulate groups of related data fields
 * in a dynamic and flexible way, enabling storage and manipulation of complex objects
 * within the virtual machine or interpreter.
 */
struct StructValue {
    /**
     * A map that stores named fields and their corresponding values in a structured type.
     *
     * This variable represents the key-value pairs within a `StructValue` object.
     * Each key in the map is a `std::string` that identifies a field name, while
     * the value is a `std::shared_ptr` to a `StackValue`, allowing dynamic and
     * polymorphic storage of various data types. This structure is crucial for
     * handling structured objects and their properties within the system.
     */
    std::unordered_map<std::string, std::shared_ptr<StackValue>> fields;
};

/**
 * Constructs an ArrayValue instance by initializing its elements with the provided vector of StackValue objects.
 * Each StackValue in the given vector is moved into a shared pointer and stored in the `elements` container,
 * ensuring efficient memory management and shared ownership of the stored values.
 *
 * @param src A vector of StackValue objects to be transformed and stored in the ArrayValue instance.
 *            The elements are moved into shared pointers within the internal `elements` container.
 */
inline ArrayValue::ArrayValue(std::vector<StackValue> src) {
    elements.reserve(src.size());
    for (auto &v : src) {
        elements.push_back(std::make_shared<StackValue>(std::move(v)));
    }
}


#endif //COMMON_H
