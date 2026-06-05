# Iris 兼容矩阵 (1.0)

VMCR 1.0 目标兼容 Iris 0.9 / 0.10 的大部分常用 shader pack.

## 渲染特性兼容表

| 特性 | Iris | VMCR 1.0 | 备注 |
| :--- | :---: | :---: | :--- |
| 光照（基础） | ✔ | ✔ | 算法移植 |
| 光照（Spec） | ✔ | ✔ | |
| 阴影映射 | ✔ | ✔ | CSM × 3 |
| AO（顶点 + 几何） | ✔ | ✔ | |
| 法线贴图 | ✔ | ✔ | |
| Bloom | ✔ | ✘ | 后续 |
| DOF | ✔ | ✘ |  |
| Motion Blur | ✔ | ✘ |  |
| SSR | ✔ | ✘ |  |
| TAA | ✔ | ✘ |  |
| 光线追踪 | ✔ | ✘ | 1.0 不包含 |

## shader pack 兼容

| Pack | 兼容 |
| :--- | :---: |
| BSL | ✔ |
| ComplementaryReimagined | ✔ |
| SEUS | ✘（需要 OptiFine） |
| Sildur's | ✔ |
| Chocapic | ✔ |

## 后续计划 (1.1+)

* 编译 Iris 的 GLSL 源为 SPIR-V, 通过 `VMCR_ENABLE_SHADER_HOT_RELOAD=ON` 加载
* 实现 optifine 风格的 uniforms 兼容
