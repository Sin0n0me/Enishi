#pragma once
#include <cstddef>
#include <cstdint>

// Private ABI; all returned pointers borrow from the execution object.
extern "C" {
struct MusubuExecution;
struct MusubuValue;
struct MusubuView {
    std::uint32_t kind;
    std::int64_t signed_value;
    std::uint64_t unsigned_value;
    double number;
    const char* text;
    std::size_t text_length;
    const char* type_name;
    std::size_t type_length;
    const char* name;
    std::size_t name_length;
    std::size_t child_count;
};
MusubuExecution* enishi_musubu_execute(const char* source, std::size_t length);
void enishi_musubu_free(MusubuExecution* execution);
const char* enishi_musubu_error(const MusubuExecution* execution, std::size_t* length);
const MusubuValue* enishi_musubu_value(const MusubuExecution* execution);
const MusubuValue* enishi_musubu_child(const MusubuValue* value, std::size_t index);
void enishi_musubu_view(const MusubuValue* value, MusubuView* view);
}
