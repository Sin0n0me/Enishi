#include "script_engine.h"
#include "musubu_bridge.h"
#include <algorithm>

namespace enishi::script {
    namespace {
        sub_system::ScriptValue copy_value(const MusubuValue* value) {
            MusubuView view{};
            enishi_musubu_view(value, &view);
            sub_system::ScriptValue result;
            result.kind = static_cast<sub_system::ScriptValueKind>(view.kind);
            result.type_name.assign(view.type_name, view.type_length);
            result.name.assign(view.name, view.name_length);
            switch (result.kind) {
                case sub_system::ScriptValueKind::Boolean:
                case sub_system::ScriptValueKind::Range:
                    result.scalar = view.unsigned_value != 0;
                    break;
                case sub_system::ScriptValueKind::SignedInteger:
                    result.scalar = view.signed_value;
                    break;
                case sub_system::ScriptValueKind::UnsignedInteger:
                case sub_system::ScriptValueKind::Enum:
                    result.scalar = view.unsigned_value;
                    result.variant_name.assign(view.text, view.text_length);
                    break;
                case sub_system::ScriptValueKind::Float:
                    result.scalar = view.number;
                    break;
                case sub_system::ScriptValueKind::String:
                    result.scalar = std::string(view.text, view.text_length);
                    break;
                default:
                    break;
            }
            result.children.reserve(view.child_count);
            for (std::size_t index = 0; index < view.child_count; ++index) {
                result.children.push_back(copy_value(enishi_musubu_child(value, index)));
            }
            return result;
        }
    } // namespace

    sub_system::ScriptResult<sub_system::ScriptValue> ScriptEngine::execute(
        std::string_view source) {
        const std::unique_ptr<MusubuExecution, decltype(&enishi_musubu_free)> execution(
            enishi_musubu_execute(source.empty() ? "" : source.data(), source.size()),
            enishi_musubu_free);
        if (execution == nullptr) {
            return foundation::Error(
                sub_system::ScriptError::ExecutionFailed, "Musubu execution could not be created");
        }
        std::size_t length = 0;
        const auto error = enishi_musubu_error(execution.get(), &length);
        if (length != 0) {
            return foundation::Error(
                sub_system::ScriptError::ExecutionFailed, std::string(error, length));
        }
        auto value = copy_value(enishi_musubu_value(execution.get()));
        // Snapshot receivers so a callback may register another receiver without invalidating
        // iteration.
        std::erase_if(this->receivers, [](const auto& receiver) { return receiver.expired(); });
        const auto receivers = this->receivers;
        for (const auto& receiver : receivers) {
            const auto target = receiver.lock();
            if (target == nullptr) {
                continue;
            }
            auto result = target->on_script_result(value);
            if (result.is_err()) {
                return foundation::Error(
                    sub_system::ScriptError::ReceiverFailed, result.unwrap_err().get_message());
            }
        }
        return value;
    }

    void ScriptEngine::add_result_receiver(
        std::shared_ptr<sub_system::IScriptResultReceiver> receiver) {
        if (receiver == nullptr) {
            return;
        }
        for (const auto& registered : this->receivers) {
            if (registered.lock() == receiver) {
                return;
            }
        }
        this->receivers.emplace_back(std::move(receiver));
    }
} // namespace enishi::script
