#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

// ===========================================================================
// chunk.vert - 方块渲染顶点着色器 (Phase 3 实现)
//
// 输入:
//   - in_pos: 位置 (vec3, 设备本地 SSBO 中读取)
//   - in_packed: 法线 + AO 打包 (uint32)
//   - in_uv: 纹理坐标 (vec2)
//   - in_light: 天空光 + 方块光 (uint16x2)
//
// 输出:
//   - gl_Position, varying 给 fragment shader
// ===========================================================================

layout(set = 0, binding = 0) uniform GlobalsUBO {
    mat4  view_proj;
    mat4  prev_view_proj;
    vec4  fog_color;        // .rgb 雾色, .a 雾密度
    vec4  time_cam;         // .xyz 相机, .w 时间
    uint  frame_id;
    uint  flags;            // bit0: fog, bit1: vignette, bit2: shadow
    uint  chunk_count;
    uint  vertex_offset;
} globals;

// 设备本地 SSBO: 顶点数据 (32 字节 stride, 16 字节对齐)
struct VMCRVertex {
    vec4    pos_normal;        // .xyz = pos, .w = packed normal_x (10bit)
    uint    normal_y_ao;       // .y = normal_y, .z = normal_z, ao
    vec2    uv;
    uint    light_sky;         // light (16bit) | sky (16bit)
    uint    color_tint;        // RGBA8
    uint    _pad0;
    uint    _pad1;
};
layout(set = 2, binding = 0) readonly buffer ChunkSSBO {
    VMCRVertex verts[];
} chunks;

layout(set = 1, binding = 0) uniform ChunkUBO {
    mat4  chunk_model;
    vec4  chunk_origin;        // .xz = chunk pos, .y = lod
    uint  material_id;         // 0/1/2
    uint  biome_tint;
} chunk_ubo;

layout(location = 0) in uint  in_vertex_id;
layout(location = 1) in uint  in_instance_id;

layout(location = 0) out vec3  out_world_pos;
layout(location = 1) out vec3  out_normal;
layout(location = 2) out vec2  out_uv;
layout(location = 3) flat out uint  out_light_sky;
layout(location = 4) flat out uint  out_ao;
layout(location = 5) flat out uint  out_material;
layout(location = 6) flat out uint  out_color;

void main() {
    // 1) 从 SSBO 读取顶点
    uint vid = in_vertex_id + uint(globals.vertex_offset);
    VMCRVertex v = chunks.verts[vid];

    // 2) 解包 normal
    mediump int nx = int(v.pos_normal.w) & 0x3FF;
    mediump int ny = int(v.normal_y_ao >> 20) & 0x3FF;
    mediump int nz = int((v.normal_y_ao >> 10) & 0x3FF) - 512;
    out_normal = normalize(vec3(float(nx - 512), float(ny - 512), float(nz)) / 511.0);

    // 3) 世界坐标
    vec3 world_pos = v.pos_normal.xyz + chunk_ubo.chunk_origin.xyz;
    out_world_pos = world_pos;

    // 4) UV / 材质
    out_uv = v.uv;
    out_material = chunk_ubo.material_id;
    out_color = v.color_tint;

    // 5) 光照 (light: 16bit, sky: 16bit)
    out_light_sky = v.light_sky;
    out_ao = (v.normal_y_ao & 0x3);

    // 6) gl_Position
    gl_Position = globals.view_proj * vec4(world_pos, 1.0);
}
