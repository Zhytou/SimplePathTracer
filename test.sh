# 建立编译文件夹
echo "preparing"
if [  -d "./build" ]; then
  rm -rf ./build
fi
mkdir ./build 

# 编译项目
echo "compiling"
cd ./build
cmake ../
make sre

# 进行测试
echo "testing"
./sre ../example/cornell-box/ cornell-box
./sre ../example/veach-mis/ veach-mis
./sre ../example/staircase/ staircase
