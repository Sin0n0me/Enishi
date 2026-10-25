#pragma once

namespace enishi::assets_system::vrm_schema {
    namespace key {
        inline constexpr char BONE[] = "bone";
        inline constexpr char HUMAN_BONES[] = "humanBones";
        inline constexpr char HUMANOID[] = "humanoid";
        inline constexpr char NODE[] = "node";
        inline constexpr char WEIGHT[] = "weight";
        inline constexpr char MESH[] = "mesh";
        inline constexpr char INDEX[] = "index";
        inline constexpr char MATERIAL[] = "material";
        inline constexpr char TARGET_VALUE[] = "targetValue";
        inline constexpr char TYPE[] = "type";
        inline constexpr char NAME[] = "name";
        inline constexpr char IS_BINARY[] = "isBinary";
        inline constexpr char OVERRIDE_BLINK[] = "overrideBlink";
        inline constexpr char OVERRIDE_LOOK_AT[] = "overrideLookAt";
        inline constexpr char OVERRIDE_MOUTH[] = "overrideMouth";
        inline constexpr char EXPRESSIONS[] = "expressions";
        inline constexpr char PRESET[] = "preset";
        inline constexpr char CUSTOM[] = "custom";
        inline constexpr char MATERIAL_COLOR_BINDS[] = "materialColorBinds";
        inline constexpr char MORPH_TARGET_BINDS[] = "morphTargetBinds";
        inline constexpr char TEXTURE_TRANSFORM_BINDS[] = "textureTransformBinds";
        inline constexpr char OFFSET[] = "offset";
        inline constexpr char SCALE[] = "scale";
        inline constexpr char SPEC_VERSION[] = "specVersion";
        inline constexpr char CENTER[] = "center";
        inline constexpr char COLLIDER_GROUPS[] = "colliderGroups";
        inline constexpr char COLLIDERS[] = "colliders";
        inline constexpr char DRAG_FORCE[] = "dragForce";
        inline constexpr char GRAVITY_DIR[] = "gravityDir";
        inline constexpr char GRAVITY_POWER[] = "gravityPower";
        inline constexpr char HIT_RADIUS[] = "hitRadius";
        inline constexpr char JOINTS[] = "joints";
        inline constexpr char RADIUS[] = "radius";
        inline constexpr char SHAPE[] = "shape";
        inline constexpr char SPRINGS[] = "springs";
        inline constexpr char STIFFNESS[] = "stiffness";
        inline constexpr char TAIL[] = "tail";
        inline constexpr char X[] = "x";
        inline constexpr char Y[] = "y";
        inline constexpr char Z[] = "z";
    } // namespace key
    namespace legacy_key {
        // VRM 0 uses the misspelled JSON key "stiffiness" for stiffness.
        inline constexpr char BINDS[] = "binds";
        inline constexpr char MATERIAL_VALUES[] = "materialValues";
        inline constexpr char MATERIAL_NAME[] = "materialName";
        inline constexpr char PROPERTY_NAME[] = "propertyName";
        inline constexpr char BLEND_SHAPE_GROUPS[] = "blendShapeGroups";
        inline constexpr char BLEND_SHAPE_MASTER[] = "blendShapeMaster";
        inline constexpr char PRESET_NAME[] = "presetName";
        inline constexpr char BONE_GROUPS[] = "boneGroups";
        inline constexpr char BONES[] = "bones";
        inline constexpr char SECONDARY_ANIMATION[] = "secondaryAnimation";
        inline constexpr char STIFFNESS[] = "stiffiness";
    } // namespace legacy_key
    namespace extension {
        inline constexpr char VRM_0[] = "VRM";
        inline constexpr char VRM_1[] = "VRMC_vrm";
        inline constexpr char SPRING_BONE[] = "VRMC_springBone";
        inline constexpr char MTOON[] = "VRMC_materials_mtoon";
        inline constexpr char KHR_MATERIALS_EMISSIVE_STRENGTH[] = "KHR_materials_emissive_strength";
        inline constexpr char KHR_MATERIALS_UNLIT[] = "KHR_materials_unlit";
        inline constexpr char KHR_MESH_QUANTIZATION[] = "KHR_mesh_quantization";
        inline constexpr char KHR_TEXTURE_TRANSFORM[] = "KHR_texture_transform";
    } // namespace extension
    namespace preset {
        inline constexpr char AA[] = "aa";
        inline constexpr char IH[] = "ih";
        inline constexpr char OU[] = "ou";
        inline constexpr char EE[] = "ee";
        inline constexpr char OH[] = "oh";
        inline constexpr char HAPPY[] = "happy";
        inline constexpr char SAD[] = "sad";
        inline constexpr char RELAXED[] = "relaxed";
        inline constexpr char BLINK[] = "blink";
        inline constexpr char BLINK_LEFT[] = "blinkLeft";
        inline constexpr char BLINK_RIGHT[] = "blinkRight";
        inline constexpr char LOOK_UP[] = "lookUp";
        inline constexpr char LOOK_DOWN[] = "lookDown";
        inline constexpr char LOOK_LEFT[] = "lookLeft";
        inline constexpr char LOOK_RIGHT[] = "lookRight";
    } // namespace preset
    namespace legacy_preset {
        inline constexpr char A[] = "a";
        inline constexpr char I[] = "i";
        inline constexpr char U[] = "u";
        inline constexpr char E[] = "e";
        inline constexpr char O[] = "o";
        inline constexpr char JOY[] = "joy";
        inline constexpr char SORROW[] = "sorrow";
        inline constexpr char FUN[] = "fun";
        inline constexpr char BLINK_L[] = "blink_l";
        inline constexpr char BLINK_R[] = "blink_r";
        inline constexpr char LOOKUP[] = "lookup";
        inline constexpr char LOOKDOWN[] = "lookdown";
        inline constexpr char LOOKLEFT[] = "lookleft";
        inline constexpr char LOOKRIGHT[] = "lookright";
        inline constexpr char UNKNOWN[] = "unknown";
    } // namespace legacy_preset
    namespace material_property {
        inline constexpr char COLOR[] = "color";
        inline constexpr char OUTLINE_COLOR[] = "outlineColor";
        inline constexpr char SHADE_COLOR[] = "shadeColor";
    } // namespace material_property
    namespace legacy_material_property {
        inline constexpr char MAIN_TEXTURE_TRANSFORM[] = "_MainTex_ST";
        inline constexpr char COLOR[] = "_Color";
        inline constexpr char OUTLINE_COLOR[] = "_OutlineColor";
        inline constexpr char SHADE_COLOR[] = "_ShadeColor";
    } // namespace legacy_material_property
    namespace override_mode {
        inline constexpr char BLOCK[] = "block";
        inline constexpr char BLEND[] = "blend";
    } // namespace override_mode
    namespace collider_shape {
        inline constexpr char SPHERE[] = "sphere";
        inline constexpr char CAPSULE[] = "capsule";
    } // namespace collider_shape
    namespace material_target {
        inline constexpr char DIFFUSE[] = "diffuse";
        inline constexpr char OUTLINE_COLOR[] = "outline_color";
        inline constexpr char AMBIENT[] = "ambient";
    } // namespace material_target
    namespace value {
        inline constexpr char SPEC_VERSION_1[] = "1.0";
        inline constexpr char NO_CENTER[] = "-1";
    } // namespace value
} // namespace enishi::assets_system::vrm_schema
