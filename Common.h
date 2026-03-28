#ifndef COMMON_H
#define COMMON_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

struct EzTable;
struct EzUserData;
struct EzEnvironment;
using EzTablePtr = std::shared_ptr<EzTable>;
using EzUserDataPtr = std::shared_ptr<EzUserData>;
using EzEnvironmentPtr = std::shared_ptr<EzEnvironment>;

using StackValue = std::variant<
    std::monostate,
    int,
    double,
    std::string,
    bool,
    EzTablePtr,
    EzUserDataPtr
>;

struct EzTableKey {
    std::variant<int, std::string> value;

    EzTableKey() = default;
    EzTableKey(const int v) : value(v) {}
    EzTableKey(std::string v) : value(std::move(v)) {}
    EzTableKey(const char* v) : value(std::string(v)) {}

    bool operator==(const EzTableKey& other) const = default;
};

struct EzTableKeyHash {
    size_t operator()(const EzTableKey& key) const {
        if (std::holds_alternative<int>(key.value)) {
            return std::hash<int>{}(std::get<int>(key.value));
        }

        return std::hash<std::string>{}(std::get<std::string>(key.value));
    }
};

struct EzTable {
    std::unordered_map<EzTableKey, std::shared_ptr<StackValue>, EzTableKeyHash> entries;

    EzTable() = default;
    explicit EzTable(std::vector<StackValue> src);

    [[nodiscard]] size_t arrayEntryCount() const;
};

struct EzUserData {
    std::shared_ptr<void> handle;
    std::string typeName;
};

struct EzEnvironment {
    EzTablePtr table = std::make_shared<EzTable>();
    std::unordered_set<std::string> constNames;
    EzEnvironmentPtr parent;
};

inline EzTable::EzTable(std::vector<StackValue> src) {
    entries.reserve(src.size());

    for (size_t i = 0; i < src.size(); ++i) {
        entries.emplace(EzTableKey{static_cast<int>(i)}, std::make_shared<StackValue>(std::move(src[i])));
    }
}

inline size_t EzTable::arrayEntryCount() const {
    size_t count = 0;

    for (const auto& [key, _] : entries) {
        if (std::holds_alternative<int>(key.value)) {
            ++count;
        }
    }

    return count;
}

#endif //COMMON_H
