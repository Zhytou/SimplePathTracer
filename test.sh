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
make main

# 进行测试
echo "testing"
./main ../example/wood-block/ wood-block.obj wood-block.xml
./main ../example/cornell-box/ cornell-box.obj cornell-box.xml
./main ../example/veach-mis/ veach-mis.obj veach-mis.xml
./main ../example/staircase/ staircase.obj staircase.xml
