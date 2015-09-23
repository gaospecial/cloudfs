#cloudfs

编译说明：

1、安装编译依赖库
ubuntu: 
apt-get update
apt-get -y install libcurl4-openssl-dev libssl-dev pkg-config libxml2 libxml2-dev libfuse-dev libunwind8-dev

centos: 
yum -y install libcurl libcurl-devel openssl-devel fuse fuse-libs fuse-devel libxml2-devel libunwind-devel

2、make
在工程目录下运行 make ，完成后会在工程目录下生成 cloudfs 二进制文件

安装运行请参考cloudfs安装指导书下的指导书。

欢迎使用。
