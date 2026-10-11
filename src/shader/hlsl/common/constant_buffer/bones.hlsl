cbuffer Bones {
    column_major matrix bone_matrices[512];
};

static const uint SKIN_LINEAR = 0u;
static const uint SKIN_DUAL_QUATERNION = 1u;
static const uint SKIN_SPHERICAL = 2u;
static const int INFLUENCE_COUNT = 4;
static const float QUATERNION_EPSILON = 0.000001;
static const float SLERP_LINEAR_THRESHOLD = 0.9995;

float4 quaternion_product(float4 a, float4 b) {
    return float4(a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz),
        a.w * b.w - dot(a.xyz, b.xyz));
}

float3 quaternion_rotate(float4 q, float3 p) {
    return p + 2.0 * cross(q.xyz, cross(q.xyz, p) + q.w * p);
}

// Input columns are the orthonormal basis of a rigid bone transform.
float4 rotation_quaternion(float3 x, float3 y, float3 z) {
    float trace = x.x + y.y + z.z;
    float4 q;
    if (trace > 0.0) {
        float s = sqrt(trace + 1.0) * 2.0;
        q = float4(y.z - z.y, z.x - x.z, x.y - y.x, 0.25 * s * s) / s;
    } else if (x.x > y.y && x.x > z.z) {
        float s = sqrt(1.0 + x.x - y.y - z.z) * 2.0;
        q = float4(0.25 * s * s, y.x + x.y, z.x + x.z, y.z - z.y) / s;
    } else if (y.y > z.z) {
        float s = sqrt(1.0 + y.y - x.x - z.z) * 2.0;
        q = float4(y.x + x.y, 0.25 * s * s, z.y + y.z, z.x - x.z) / s;
    } else {
        float s = sqrt(1.0 + z.z - x.x - y.y) * 2.0;
        q = float4(z.x + x.z, z.y + y.z, 0.25 * s * s, x.y - y.x) / s;
    }
    return normalize(q);
}


float4 bone_rotation(uint index) {
    float4x4 m = bone_matrices[index];
    return rotation_quaternion(float3(m[0][0], m[1][0], m[2][0]),
        float3(m[0][1], m[1][1], m[2][1]), float3(m[0][2], m[1][2], m[2][2]));
}
float3 bone_point(uint index, float3 p) { return mul(bone_matrices[index], float4(p, 1.0)).xyz; }
float3 bone_direction(uint index, float3 p) { return mul((float3x3)bone_matrices[index], p); }
float3 bone_translation(uint index) { return bone_point(index, float3(0.0, 0.0, 0.0)); }
float4 spherical_rotation(float4 a, float4 b, float weight) {
    float cosine = dot(a, b);
    if (cosine < 0.0) {
        b = -b;
        cosine = -cosine;
    }
    if (cosine > SLERP_LINEAR_THRESHOLD) {
        return normalize(lerp(a, b, weight));
    }
    float angle = acos(clamp(cosine, -1.0, 1.0));
    return (sin((1.0 - weight) * angle) * a + sin(weight * angle) * b) / sin(angle);
}

struct DeformedVertex {
    float3 position;
    float3 normal;
};

DeformedVertex deform_vertex(float3 position, float3 normal, uint4 bones, float4 weights,
    uint method, float3 center, float3 anchor0, float3 anchor1) {
    DeformedVertex output;
    output.position = position;
    output.normal = normal;
    if (method == SKIN_SPHERICAL) {
        float4 rotation = spherical_rotation(bone_rotation(bones.x), bone_rotation(bones.y), weights.y);
        output.position = quaternion_rotate(rotation, position - center)
            + bone_point(bones.x, anchor0) * weights.x
            + bone_point(bones.y, anchor1) * weights.y;
        output.normal = quaternion_rotate(rotation, normal);
    } else if (method == SKIN_DUAL_QUATERNION) {
        float4 real_part = float4(0.0, 0.0, 0.0, 0.0);
        float4 dual_part = float4(0.0, 0.0, 0.0, 0.0);
        float4 reference = float4(0.0, 0.0, 0.0, 1.0);
        bool has_reference = false;
        for (int influence = 0; influence < INFLUENCE_COUNT; ++influence) {
            if (weights[influence] == 0.0) {
                continue;
            }
            uint bone = bones[influence];
            float4 rotation = bone_rotation(bone);
            if (!has_reference) {
                reference = rotation;
                has_reference = true;
            }
            float weight = weights[influence];
            if (dot(reference, rotation) < 0.0) {
                weight = -weight;
            }
            real_part += rotation * weight;
            dual_part += quaternion_product(float4(bone_translation(bone), 0.0), rotation) * (0.5 * weight);
        }
        float magnitude = length(real_part);
        if (magnitude > QUATERNION_EPSILON) {
            real_part /= magnitude;
            dual_part /= magnitude;
            float4 conjugate_rotation = float4(-real_part.xyz, real_part.w);
            float3 translation = 2.0 * quaternion_product(dual_part, conjugate_rotation).xyz;
            output.position = quaternion_rotate(real_part, position) + translation;
            output.normal = quaternion_rotate(real_part, normal);
        }
    } else {
        output.position = float3(0.0, 0.0, 0.0);
        output.normal = float3(0.0, 0.0, 0.0);
        for (int influence = 0; influence < INFLUENCE_COUNT; ++influence) {
            if (weights[influence] != 0.0) {
                output.position += bone_point(bones[influence], position) * weights[influence];
                output.normal += bone_direction(bones[influence], normal) * weights[influence];
            }
        }
    }
    return output;
}
