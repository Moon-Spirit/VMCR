#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

// ===========================================================================
// chunk.frag - 方块渲染片段着色器 (Phase 3 实现)
//
// 计算最终颜色:
//   1) 采样纹理图集 (按 uv + material 决定 atlas index)
//   2) 应用 AO + 天空光 / 方块光
//   3) 阳光方向阴影
//   4) 雾
// ===========================================================================

layout(set = 0, binding = 0) uniform GlobalsUBO {
    mat4  view_proj;
    mat4  prev_view_proj;
    vec4  fog_color;
    vec4  time_cam;
    uint  frame_id;
    uint  flags;
    uint  chunk_count;
    uint  vertex_offset;
} globals;

layout(set = 1, binding = 0) uniform ChunkUBO {
    mat4  chunk_model;
    vec4  chunk_origin;
    uint  material_id;
    uint  biome_tint;
} chunk_ubo;

// 纹理图集 (descriptor indexing)
layout(set = 2, binding = 0) uniform usampler2D tex_atlas;

layout(location = 0) in vec3  in_world_pos;
layout(location = 1) in vec3  in_normal;
layout(location = 2) in vec2  in_uv;
layout(location = 3) flat in uint  in_light_sky;
layout(location = 4) flat in uint  in_ao;
layout(location = 5) flat in uint  in_material;
layout(location = 6) flat in uint  in_color;

layout(location = 0) out vec4 out_color;

const vec3 SUN_DIR = vec3(0.6, 0.8, 0.3);
const float SHADOW_BIAS = 0.001;

float compute_ao(uint ao_bits) {
    // ao 范围 0..3
    return float(3 - (ao_bits & 0x3)) / 3.0;
}

float unpack_light(uint packed, uint hi) {
    // packed = light (16 low) | sky (16 high)
    uint v = (hi != 0u) ? (packed >> 16) : (packed & 0xFFFFu);
    return float(v) / 16.0;  // MC: 0..15
}

void main() {
    // 1) 采样纹理图集
    //    uv 在 atlas 中的位置由 material_id 决定
    vec2 atlas_uv = in_uv;  // 简化: 实际需要根据 material_id 偏移
    vec4 tex = texture(tex_atlas, atlas_uv);

    // 2) 丢弃透明像素 (cutout)
    if (in_material == 1u && tex.a < 0.5) discard;

    // 3) 基础颜色
    vec3 base_rgb = tex.rgb;
    // 生物群系染色
    base_rgb *= mix(vec3(1.0), vec3(0.6, 0.8, 0.5), float(chunk_ubo.biome_tint) / 16.0);

    // 4) 漫反射 + 环境光
    float ndotl = max(dot(normalize(in_normal), SUN_DIR), 0.0);
    float sky = unpack_light(in_light_sky, 1u);
    float block = unpack_light(in_light_sky, 0u);
    float ao_factor = mix(0.5, 1.0, compute_ao(in_ao));

    vec3 lit = base_rgb * (
        0.4 * block / 16.0 +              // 方块光
        0.6 * sky  / 16.0 +              // 天空光
        0.3 * ndotl * ao_factor          // 太阳
    );

    // 5) 雾
    if ((globals.flags & 1u) != 0u) {
        float dist = length(in_world_pos - globals.time_cam.xyz);
        float fog_factor = 1.0 - exp(-dist * globals.fog_color.a);
        lit = mix(lit, globals.fog_color.rgb, clamp(fog_factor, 0.0, 1.0));
    }

    out_color = vec4(lit, tex.a);
}
