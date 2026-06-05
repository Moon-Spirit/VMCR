# ===========================================================================
# third_party/ - VMCR 第三方依赖
# 通过 git submodule 引入
#
# 初始化:
#   git submodule update --init --recursive
#
# 如需更新:
#   git submodule update --remote
#
# 注: 国内网络可设置代理或使用镜像:
#   git config --global url."https://ghproxy.com/https://github.com/".insteadOf "https://github.com/"
# ===========================================================================

# 核心 Vulkan 生态
Vulkan-Hpp/         # KhronosGroup/Vulkan-Hpp          C++ Vulkan 绑定 (RAII)
VMA/                # GPUOpen-LibrariesAndSDKs/VMA     Vulkan 显存分配器
vkbootstrap/        # charles-lunarg/vk-bootstrap       实例/设备/交换链封装

# Shader 工具链
glslang/             # KhronosGroup/glslang            GLSL → SPIR-V
SPIRV-Cross/         # KhronosGroup/SPIRV-Cross        SPIR-V 反射/反编译 (Iris 兼容)

# 数学
glm/                # g-truc/glm                       矩阵/向量/四元数 (Y 轴反转)

# 日志
spdlog/              # gabime/spdlog                   高性能日志, 输出到 logcat

# Hook (可选)
dobby/               # jmpews/Dobby                    运行时 hook 框架
