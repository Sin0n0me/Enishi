#pragma once
#include <tuple>

namespace enishi::ecs {
    template <typename... Ts> class View {
      private:
        std::tuple<ComponentPool<Ts>&...> pools;
        std::uint32_t current_id;

      private:
        // 全コンポーネントを持つエンティティまでスキップ
        void skip_invalid(const uint32_t start) {
            this->current_id = start;
            auto& entities = this->base_pool().entities();
            const auto entities_size = entities.size();

            for (; this->current_id < entities_size; ++this->current_id) {
                if (!this->has_all(entities[this->current_id])) {
                    break;
                }
            }
        }

        bool has_all(const EntityID id) const {
            return (... && std::get<ComponentPool<Ts>&>(pools).has(id));
        }

        auto& base_pool(void) {
            return std::get<ComponentPool<std::tuple_element_t<0, std::tuple<Ts...>>>&>(
                this->pools);
        }

      public:
        explicit View(ComponentPool<Ts>&... pools)
            : current_id(0)
            , pools(pools...) {
        }

        // range-based for で使えるイテレータ
        struct Iterator {
            View& view;
            uint32_t idx;

            bool operator!=(const Iterator& other) const {
                return this->idx != other.idx;
            }
            Iterator& operator++(void) {
                ++this->idx;
                return *this;
            }

            // [EntityID, Component&...] のタプルを返す
            std::tuple<EntityID, Ts&...> operator*(void) {
                // 最もエンティティ数が少ないPoolのdenseを基準にする
                EntityID id = this->base_pool().entities()[this->idx];

                return std::tuple<EntityID, Ts&...>(
                    id, std::get<ComponentPool<Ts>&>(this->view.pools).get(id).unwrap_mut()...);
            }

            ComponentPool<std::tuple_element_t<0, std::tuple<Ts...>>>& base_pool(void) {
                return std::get<0>(this->view.pools);
            }
        };

        Iterator begin(void) {
            this->skip_invalid(0);
            return Iterator{*this, this->current_id};
        }

        Iterator end(void) {
            return Iterator{*this, static_cast<uint32_t>(this->base_pool().entities().size())};
        }
    };
} // namespace enishi::ecs