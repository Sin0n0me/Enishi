#include "material.h"

namespace enishi::types {
    UniformMaterial make_uniform_material(const Material& material) {
        UniformMaterial uniform{};
        uniform.diffuse = glm::vec4(1);
        std::size_t scalar_index = 0;
        for (const auto& variant : material.variants) {
            if (const auto* diffuse = std::get_if<Diffuse>(&variant); diffuse != nullptr) {
                uniform.diffuse = diffuse->color;
            } else if (const auto* specular = std::get_if<Specular>(&variant);
                       specular != nullptr) {
                uniform.specular = specular->color;
                uniform.shininess = glm::vec1(specular->shininess);
            } else if (const auto* ambient = std::get_if<Ambient>(&variant); ambient != nullptr) {
                uniform.ambient = ambient->color;
            } else if (const auto* scalar = std::get_if<glm::vec1>(&variant); scalar != nullptr) {
                // The legacy material variants store environment blending and edge enable
                // after the lighting properties. Keep their existing uniform meanings.
                constexpr std::size_t MULTIPLY = 0;
                constexpr std::size_t ADD = 1;
                constexpr std::size_t EDGE = 2;
                if (scalar_index == MULTIPLY) {
                    uniform.sphere_mul = *scalar;
                } else if (scalar_index == ADD) {
                    uniform.sphere_add = *scalar;
                } else if (scalar_index == EDGE) {
                    uniform.edge_flag = *scalar;
                }
                ++scalar_index;
            }
        }
        uniform.outline_color = material.outline_color;
        uniform.outline_parameters.x = material.outline_width;
        return uniform;
    }
} // namespace enishi::types
