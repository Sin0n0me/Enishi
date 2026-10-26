#pragma once
#include <memory>
namespace enishi::platform {
    class IRenderer;
}
bool render_pass_test(const std::shared_ptr<enishi::platform::IRenderer>& renderer);
