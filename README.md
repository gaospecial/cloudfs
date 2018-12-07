# cloudfs：挂载阿里云 OSS 到 ECS

## 使用说明

阿里云的 ECS 磁盘是要收费的，很贵。

阿里云的 OSS 存储是按照流量收费，且**内网流量**不收费。

如果将青岛的 OSS 通过阿里云内网挂载到同一机房的 ECS 上面，那么你的 ECS 就会增加很多可用的免费空间喽(你只需要付 ECS 的流量费就行了)！

cloudfs 自 2016年1月 之后，再没有更新，原来的[公司链接](http://www.cloudtalkers.com/)也打不开了。

更新 ECS 系统版本到 Ubuntu 16.04 之后，发现 cloudfs 用不了了。这让我很尴尬。因为我把图片、视频什么的都是存在 OSS，然后链接到网站上面的。

随后发现编译 cloudfs 之后其实也是可以用的，只是原来的安装文件不兼容新系统而已。

经过一晚上的摸索，总算是成功解决了 cloudfs 在 Ubuntu 16.04 上面不能用的问题(具体步骤参见：<INSTALL.md>)。

虽然没有测试，但感觉上面的这个安装方法也适用于其它发行版(编译 + systemctl 工具具有普适性)。

-----------

下面是原版 README 对 cloudfs 的说明。

## 软件特点：
### 1、速度快
    利用多线程+MultiPart机制, 大文件写入OSS可以达到50MB~100MB每秒, 可充分利用ECS和OSS之间的内网带宽;

### 2、功能齐全
    支持FTP/rsync/sftp/scp等传输软件的断点续传;
    支持用户修改文件权限/文件时间;
    支持OSS下载的content-type由文件后缀智能识别定义, 同时支持用户自定义扩展;
    支持Linux软链接文件;

### 3、大文件支持
    单文件最大可以支持到OSS上限45TB;
    可支持数百万级别的文件管理;

### 4、已与多种流行应用集成
    Discuz/Wordpres/ownCloud/rsync/sftp/scp/apache/nginx/以及多种私有应用系统;

### 5、稳定
    已有数百位爱好者进行使用验证, 部分已经生产上线;

### 6、操作简单, 可以天然无缝适配已有程序;

### 7、代码结构清晰, 可充分扩展;

## 欢迎使用

### 用户支持交流群：307228533
