#pragma once
#include <foundation/result/result.h>

namespace enishi::ecs {
    enum class ECSError {
        AlreadyHasComponent,
    };

    template <typename T> using ECSReuslt = foundation::Result<T, ECSError>;
} // namespace enishi::ecs