# Simple Path Tracer

[![wakatime](https://wakatime.com/badge/github/Zhytou/SimplePathTracer.svg)](https://wakatime.com/badge/github/Zhytou/SimplePathTracer)

**编译运行**：

```bash
# 克隆项目
git clone git@github.com:Zhytou/SimplePathTracer.git

# 下载第三方库
git submodule update --init

# 使用CMake编译
mkdir build
cd build
cmake ../

# 渲染康奈尔盒模型
make main
./main
```

**渲染效果**：

![res](res/box_spp32_mis_492s.png)

![res](res/metal-box_spp32_mis_462s.png)

![res](res/glossy-metal-box_spp32_mis_450s.png)

![res](res/glass-panel_spp32_mis_518s.png)

![res](res/dragon_spp32_mis_1641s.png)

![res](res/staircase_spp32_mis_1796s.png)

![res](res/veach-mis_spp32_mis_2434s.png)