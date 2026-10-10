#pragma once

namespace enishi::core {
    enum class ModelClickMode { None, Touch, Push };

    struct ModelClickSettings {
        ModelClickMode mode = ModelClickMode::Push;
    };
} // namespace enishi::core
