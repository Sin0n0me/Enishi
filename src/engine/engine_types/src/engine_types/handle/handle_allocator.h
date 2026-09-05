#pragma once
#include "handle_type.h"
#include <vector>

namespace enishi::types {
    class HandleAllocator {
      private:
        HandleId next_id;
        std::vector<bool> alive;
        std::vector<HandleId> free_list;

      public:
        explicit HandleAllocator(void);

        HandleId create(void);

        void destroy(const HandleId id);

        bool is_alive(const HandleId id) const noexcept;
    };
} // namespace enishi::types