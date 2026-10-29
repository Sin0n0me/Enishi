#include "effect_manager.h"
#include <limits>
#include <utility>

namespace enishi::effect {
    namespace {
        class OperationGuard {
          private:
            bool& busy;

          public:
            explicit OperationGuard(bool& busy);
            ~OperationGuard();
            OperationGuard(const OperationGuard&) = delete;
            OperationGuard& operator=(const OperationGuard&) = delete;
        };
        OperationGuard::OperationGuard(bool& busy)
            : busy(busy) {
            this->busy = true;
        }
        OperationGuard::~OperationGuard() {
            this->busy = false;
        }
        foundation::Error<EffectError> busy_error() {
            return {EffectError::Busy, "Effect lifecycle operation is already in progress"};
        }
    } // namespace
    struct EffectManager::Entry {
        std::shared_ptr<IEffect> effect;
        explicit Entry(std::shared_ptr<IEffect> effect);
        ~Entry();
    };
    EffectManager::Entry::Entry(std::shared_ptr<IEffect> effect)
        : effect(std::move(effect)) {
        this->effect->managed = true;
    }
    EffectManager::Entry::~Entry() {
        this->effect->stop();
        this->effect->managed = false;
    }
    EffectManager::EffectManager() = default;
    EffectManager::~EffectManager() noexcept {
        // The owner must not destroy the manager from inside an effect callback.
        static_cast<void>(this->stop_all());
    }
    foundation::Result<EffectId, EffectError> EffectManager::play(std::unique_ptr<IEffect> effect) {
        return this->play_shared(std::shared_ptr<IEffect>{std::move(effect)});
    }
    foundation::Result<EffectId, EffectError> EffectManager::play_shared(
        std::shared_ptr<IEffect> effect) {
        if (this->busy) {
            return busy_error();
        }
        if (effect == nullptr) {
            return foundation::Error{EffectError::InvalidArgument, "Effect must not be null"};
        }
        if (effect->managed) {
            return foundation::Error{EffectError::Busy, "Effect is already managed by a player"};
        }
        if (this->next_id == EffectId::INVALID_VALUE) {
            return foundation::Error{EffectError::IdExhausted, "Effect IDs are exhausted"};
        }
        const OperationGuard guard(this->busy);
        const EffectId id{this->next_id};
        // Never wrap the counter back to an ID belonging to a previous playback.
        if (this->next_id == std::numeric_limits<EffectId::Value>::max()) {
            this->next_id = EffectId::INVALID_VALUE;
        } else {
            ++this->next_id;
        }
        auto entry = std::make_unique<Entry>(std::move(effect));
        auto started = entry->effect->start();
        if (started.is_err()) {
            return std::move(started).take_err();
        }
        this->entries.emplace(id.value, std::move(entry));
        return id;
    }
    foundation::Result<bool, EffectError> EffectManager::stop(EffectId id) {
        if (this->busy) {
            return busy_error();
        }
        const OperationGuard guard(this->busy);
        // Remove visibility before callbacks so is_playing() is safe during cleanup.
        auto entry = this->entries.extract(id.value);
        return !entry.empty();
    }
    foundation::Result<void, EffectError> EffectManager::stop_all() {
        if (this->busy) {
            return busy_error();
        }
        const OperationGuard guard(this->busy);
        while (!this->entries.empty()) {
            auto entry = this->entries.extract(this->entries.begin());
        }
        return {};
    }
    bool EffectManager::is_playing(EffectId id) const noexcept {
        return this->entries.contains(id.value);
    }
    foundation::Result<std::vector<EffectFailure>, EffectError> EffectManager::update(
        const types::DeltaTime& delta_time) {
        if (this->busy) {
            return busy_error();
        }
        if (delta_time.delta_time.count() < 0) {
            return foundation::Error{
                EffectError::InvalidArgument, "Effect delta time must not be negative"};
        }
        const OperationGuard guard(this->busy);
        std::vector<EffectFailure> failures;
        for (auto iterator = this->entries.begin(); iterator != this->entries.end();) {
            const auto current = iterator++;
            auto updated = current->second->effect->update(delta_time);
            if (updated.is_err()) {
                failures.push_back({EffectId{current->first}, std::move(updated).take_err()});
                auto entry = this->entries.extract(current);
            } else if (updated.unwrap() == EffectStatus::Finished) {
                auto entry = this->entries.extract(current);
            }
        }
        return failures;
    }
} // namespace enishi::effect
