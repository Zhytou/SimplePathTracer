# Simple Render Engine

[![wakatime](https://wakatime.com/badge/github/Zhytou/SimpleRenderEngine.svg)](https://wakatime.com/badge/github/Zhytou/SimpleRenderEngine)

- [Simple Render Engine](#simple-render-engine)
  - [原理](#原理)
    - [路径追踪](#路径追踪)
    - [光照模型](#光照模型)
    - [视角转换](#视角转换)
  - [优化](#优化)
    - [OpenMP并行加速](#openmp并行加速)
    - [直接光照](#直接光照)
    - [BVH加速](#bvh加速)
  - [TODO List](#todo-list)
  - [References](#references)

## 原理

### 路径追踪

路径追踪的基本原理是由相机从场景中发送光线而模拟光线在三维场景中的传播和交互过程，计算出每个像素点上的颜色和亮度值。
一般来说，一个基础的路径追踪流程如下：
- 相机向屏幕中每一个像素点投射光线；
- 光线进入场景，若命中光源，则直接返回光源颜色；若命中场景中物品，则光线会根据物品材料特性发生漫反射、镜面反射或折射，并进入下一轮路径追踪，直到命中光源或超过递归深度。
  - 其中，针对漫反射情况，我们采用了蒙特卡洛积分的思想，取一球面随机向量作为递归的光线，只需在该路径追踪返回值除以pdf（1/2*pi）即可。（详细理论可参考此[博客](https://blog.csdn.net/weixin_44176696/article/details/113418991)）
  - 此外，光线还会随着传播距离发生衰减，应该除以其传播距离的平方。
- 最后，为了提高图像画质，我们可以向屏幕中每一个像素点投射多条光线。这个值也被称为Sample Pre Pixel，简称SSP。

### 光照模型

光照模型是一种用于描述物体在不同光照条件下表现的方法。它包含了描述物体表面反射和折射特性的材料属性，以及描述光源和环境光照强度的光源属性，以计算最终的颜色值。
本实验采用的是冯氏光照模型，关于冯氏模型的介绍可以参考此[网站](https://learnopengl-cn.readthedocs.io/zh/latest/02%20Lighting/02%20Basic%20Lighting/)。

简单来说，冯氏光照模型将物体发出的光分成了三个部分：自发光、漫反射光和镜面反射光（镜面反射光也被称为高光）。其中，漫反射光受物体的漫反射衰减参数（Kd）影响，而镜面反射光则受物体的高光衰减系数（Ks）影响。本实验中，这些参数由模型的mtl文件提供。因此，我们只需要使用[tinyobjloader](https://github.com/tinyobjloader/tinyobjloader/tree/release)读取并将这些参数存入相应的Material对象即可。

### 视角转换

根据

### 模型面元

## 优化

### OpenMP并行加速

由于每个像素的值是相互独立的，因此可以使用OpenMP并行库进行加速。

### 直接光照

为了减少递归超过最大深度，光线也没有命中光源的概率，我们可以使用光源重要性采样，即：每命中一个物体，就尝试判断光源是否可以命中该交点（连线无障碍），若可命中，则可将光源强度添加至直接光照中。

### BVH加速

　BVH盒全称是Bounding Volume Hierarchies，即层次包围盒。这里我们先不直接讲它的原理，我们先来回忆一下之前的内容：当我们要开始求交的时候，我们需要拿到hittable_list，然后逐个遍历每个物体。这样我们的求交复杂度是O(n)，当场景较为庞大，三角面的数量达到十万、百万级别的时候，求交会变成一个极其吃力的事情。因此，我们需要将求交的复杂度进一步降低。

## TODO List

- [x] Baisc path tracing
  - [x] Phong lighting
  - [x] Camera(perspective and orthographic)
  - [x] OBJ load/Triangle mesh/Material
- [x] Optimization
  - [x] OpenMP acceleration
  - [x] Direct lighting
  - [ ] Importance sampling(using cosine)
  - [x] BVH acceleration
- [x] Texture support
- [ ] Benchmark
- [ ] More acceleration(e.g. CUDA)

## References

- [光照模型](https://learnopengl-cn.readthedocs.io/zh/latest/02%20Lighting/02%20Basic%20Lighting/)
