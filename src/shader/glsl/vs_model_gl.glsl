#version 400 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in uvec4 in_bone_indices;
layout(location = 4) in vec4 in_bone_weights;
layout(location = 5) in uint in_skinning_method;
layout(location = 6) in vec3 in_blend_center;
layout(location = 7) in vec3 in_blend_anchor0;
layout(location = 8) in vec3 in_blend_anchor1;

layout(std140) uniform Camera {
    mat4 world;
    mat4 view;
    mat4 projection;
    mat4 mvp;
};

layout(std140) uniform Bones {
    mat4 bone_matrices[512];
};

out vec2 uv;
out vec3 view_position;
out vec3 view_normal;
out vec3 view_light;


const uint SKIN_LINEAR = 0u;
const uint SKIN_DUAL_QUATERNION = 1u;
const uint SKIN_SPHERICAL = 2u;
const int INFLUENCE_COUNT = 4;
const float QUATERNION_EPSILON = 0.000001;
const float SLERP_LINEAR_THRESHOLD = 0.9995;

vec4 quaternion_product(vec4 a, vec4 b) {
    return vec4(a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz),
        a.w * b.w - dot(a.xyz, b.xyz));
}

vec3 quaternion_rotate(vec4 q, vec3 p) {
    return p + 2.0 * cross(q.xyz, cross(q.xyz, p) + q.w * p);
}

// Input columns are the orthonormal basis of a rigid bone transform.
vec4 rotation_quaternion(vec3 x, vec3 y, vec3 z) {
    float trace = x.x + y.y + z.z;
    vec4 q;
    if (trace > 0.0) {
        float s = sqrt(trace + 1.0) * 2.0;
        q = vec4(y.z - z.y, z.x - x.z, x.y - y.x, 0.25 * s * s) / s;
    } else if (x.x > y.y && x.x > z.z) {
        float s = sqrt(1.0 + x.x - y.y - z.z) * 2.0;
        q = vec4(0.25 * s * s, y.x + x.y, z.x + x.z, y.z - z.y) / s;
    } else if (y.y > z.z) {
        float s = sqrt(1.0 + y.y - x.x - z.z) * 2.0;
        q = vec4(y.x + x.y, 0.25 * s * s, z.y + y.z, z.x - x.z) / s;
    } else {
        float s = sqrt(1.0 + z.z - x.x - y.y) * 2.0;
        q = vec4(z.x + x.z, z.y + y.z, 0.25 * s * s, x.y - y.x) / s;
    }
    return normalize(q);
}


vec4 bone_rotation(uint index) {
    mat4 m = bone_matrices[index];
    return rotation_quaternion(m[0].xyz, m[1].xyz, m[2].xyz);
}
vec3 bone_point(uint index, vec3 p) { return (bone_matrices[index] * vec4(p, 1.0)).xyz; }
vec3 bone_direction(uint index, vec3 p) { return mat3(bone_matrices[index]) * p; }
vec3 bone_translation(uint index) { return bone_matrices[index][3].xyz; }
vec4 spherical_rotation(vec4 a, vec4 b, float weight) {
    float cosine = dot(a, b);
    if (cosine < 0.0) {
        b = -b;
        cosine = -cosine;
    }
    if (cosine > SLERP_LINEAR_THRESHOLD) {
        return normalize(mix(a, b, weight));
    }
    float angle = acos(clamp(cosine, -1.0, 1.0));
    return (sin((1.0 - weight) * angle) * a + sin(weight * angle) * b) / sin(angle);
}

struct DeformedVertex {
    vec3 position;
    vec3 normal;
};

DeformedVertex deform_vertex(vec3 position, vec3 normal, uvec4 bones, vec4 weights,
    uint method, vec3 center, vec3 anchor0, vec3 anchor1) {
    DeformedVertex output;
    output.position = position;
    output.normal = normal;
    if (method == SKIN_SPHERICAL) {
        vec4 rotation = spherical_rotation(bone_rotation(bones.x), bone_rotation(bones.y), weights.y);
        output.position = quaternion_rotate(rotation, position - center)
            + bone_point(bones.x, anchor0) * weights.x
            + bone_point(bones.y, anchor1) * weights.y;
        output.normal = quaternion_rotate(rotation, normal);
    } else if (method == SKIN_DUAL_QUATERNION) {
        vec4 real_part = vec4(0.0, 0.0, 0.0, 0.0);
        vec4 dual_part = vec4(0.0, 0.0, 0.0, 0.0);
        vec4 reference = vec4(0.0, 0.0, 0.0, 1.0);
        bool has_reference = false;
        for (int influence = 0; influence < INFLUENCE_COUNT; ++influence) {
            if (weights[influence] == 0.0) {
                continue;
            }
            uint bone = bones[influence];
            vec4 rotation = bone_rotation(bone);
            if (!has_reference) {
                reference = rotation;
                has_reference = true;
            }
            float weight = weights[influence];
            if (dot(reference, rotation) < 0.0) {
                weight = -weight;
            }
            real_part += rotation * weight;
            dual_part += quaternion_product(vec4(bone_translation(bone), 0.0), rotation) * (0.5 * weight);
        }
        float magnitude = length(real_part);
        if (magnitude > QUATERNION_EPSILON) {
            real_part /= magnitude;
            dual_part /= magnitude;
            vec4 conjugate_rotation = vec4(-real_part.xyz, real_part.w);
            vec3 translation = 2.0 * quaternion_product(dual_part, conjugate_rotation).xyz;
            output.position = quaternion_rotate(real_part, position) + translation;
            output.normal = quaternion_rotate(real_part, normal);
        }
    } else {
        output.position = vec3(0.0, 0.0, 0.0);
        output.normal = vec3(0.0, 0.0, 0.0);
        for (int influence = 0; influence < INFLUENCE_COUNT; ++influence) {
            if (weights[influence] != 0.0) {
                output.position += bone_point(bones[influence], position) * weights[influence];
                output.normal += bone_direction(bones[influence], normal) * weights[influence];
            }
        }
    }
    return output;
}
void main() {
    const vec3 light_direction = normalize(vec3(0.0, 0.0, -10.0));
    DeformedVertex deformed = deform_vertex(in_position, in_normal, in_bone_indices,
        in_bone_weights, in_skinning_method, in_blend_center, in_blend_anchor0, in_blend_anchor1);
    vec4 position = vec4(deformed.position, 1.0);
    mat4 model_view = view * world;
    gl_Position = mvp * position;
    uv = in_uv;
    view_position = (model_view * position).xyz;
    view_normal = normalize(mat3(model_view) * deformed.normal);
    view_light = mat3(model_view) * light_direction;
}
