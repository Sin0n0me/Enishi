#include <cstdlib>
#include <iostream>
#include <script/script_engine.h>

using namespace enishi;
void popup_tests();
void selection_tests();
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}
namespace {
    class Receiver final : public sub_system::IScriptResultReceiver {
      public:
        int calls = 0;
        sub_system::ScriptValue value;
        bool reject = false;
        sub_system::ScriptResult<void> on_script_result(
            const sub_system::ScriptValue& value) override {
            ++this->calls;
            this->value = value;
            if (this->reject) {
                return foundation::Error(
                    sub_system::ScriptError::ReceiverFailed, "receiver failure");
            }
            return {};
        }
    };
    sub_system::ScriptValue run(script::ScriptEngine& engine, const char* source) {
        auto result = engine.execute(source);
        if (result.is_err()) {
            std::cerr << result.unwrap_err().get_message() << '\n';
        }
        require(result.is_ok(), "Valid script failed");
        return std::move(result.unwrap_mut());
    }
    void script_tests() {
        script::ScriptEngine engine;
        auto receiver = std::make_shared<Receiver>();
        engine.add_result_receiver(receiver);
        engine.add_result_receiver(receiver);
        auto value = run(engine, "fn helper() -> i32 { 7 } fn main() -> i32 { helper() * 6 }");
        require(std::get<std::int64_t>(value.scalar) == 42, "main was not selected by name");
        require(receiver->calls == 1, "Duplicate receiver notification");
        const auto saved = receiver->value;
        require(engine.execute("fn main() { missing; }").is_err(), "Compile error was accepted");
        require(engine.execute("fn main() -> i32 { let n = 0; 1 / n }").is_err(),
            "Runtime error was accepted");
        require(engine.execute("fn helper() -> i32 { 1 }").is_err(), "Missing main was accepted");
        require(
            engine.execute("fn main(x: i32) -> i32 { x }").is_err(), "main argument was accepted");
        require(receiver->calls == 1, "Failed execution notified receivers");
        require(std::get<std::int64_t>(saved.scalar) == 42,
            "Result lifetime depended on Rust execution");
        value = run(engine, "fn main() -> u64 { 18446744073709551615u64 }");
        require(std::get<std::uint64_t>(value.scalar) == UINT64_MAX, "Unsigned precision was lost");
        value = run(
            engine, "struct Move { x: f32, y: f32 } fn main() -> Move { Move { x: 1.0, y: 2.0 } }");
        require(value.kind == sub_system::ScriptValueKind::Struct && value.type_name == "Move",
            "Struct metadata missing");
        require(
            value.children.size() == 2 && value.children[0].name == "x", "Struct fields missing");
        value = run(engine, "fn main() -> vec3 { vec3(1.0, 2.0, 3.0) }");
        require(value.children.size() == 3 && std::get<double>(value.children[2].scalar) == 3.0,
            "Vector result corrupted");
        value =
            run(engine, "fn main() -> mat2x2f32 { mat2x2f32(vec2f32(1.0,2.0),vec2f32(3.0,4.0)) }");
        require(value.children.size() == 2 &&
                    std::get<double>(value.children[1].children[0].scalar) == 3.0,
            "Matrix column order corrupted");
        receiver->reject = true;
        auto failed = engine.execute("fn main() {}");
        require(failed.is_err() && failed.unwrap_err().get_message() == "receiver failure",
            "Receiver failure not propagated");
        receiver.reset();
        run(engine, "fn main() {}");
        auto error = engine.execute("fn main() {\n    missing;\n}");
        require(error.is_err() &&
                    error.unwrap_err().get_message().find("script:2:") != std::string::npos,
            "Diagnostic location missing");
    }
} // namespace
int main() {
    script_tests();
    popup_tests();
    selection_tests();
    std::cout << "Script execution, receivers, UI navigation and popup bounds passed\n";
}
