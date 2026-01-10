# Simple Path Tracer

[![wakatime](https://wakatime.com/badge/github/Zhytou/SimpleRenderEngine.svg)](https://wakatime.com/badge/github/Zhytou/SimpleRenderEngine)

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

- 片元数量32
- SPP 256
- 512*512
- 渲染时间：709s

![res](res/box-512*512-spp256.png)

- 片元数量234945
- SPP 128
- 512*512
- 渲染时间：5324s

![res](res/dragon-512*512-spp128.png)
