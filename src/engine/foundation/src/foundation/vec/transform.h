#pragma once
#include "vec.h"
#include <memory>
#include <ranges>

namespace enishi ::foundation {
    template <typename T>
    [[nodiscard]] Vec<std::shared_ptr<T>> to_shared_vec_from_unique_vec(
        Vec<std::unique_ptr<T>>&& views) {
        return views | std::views::as_rvalue | std::ranges::to<Vec<std::shared_ptr<T>>>();
    }
} // namespace enishi::foundation