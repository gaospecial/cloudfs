#cloudfs

##编译说明：

###1、安装编译依赖库
	ubuntu: 
	apt-get update
	apt-get -y install libcurl4-openssl-dev libssl-dev pkg-config libxml2 libxml2-dev libfuse-dev libunwind8-dev

	centos: 
	yum -y install libcurl libcurl-devel openssl-devel fuse fuse-libs fuse-devel libxml2-devel libunwind-devel

###2、make
	在工程目录下运行 make ，完成后会在工程目录下生成 cloudfs 二进制文件

###安装运行请参考cloudfs安装指导书下的指导书。


##软件特点：
###1、速度快 
    利用多线程+MultiPart机制, 大文件写入OSS可以达到50MB~100MB每秒, 可充分利用ECS和OSS之间的内网带宽;

###2、功能齐全     
	支持FTP/rsync/sftp/scp等传输软件的断点续传;
    支持用户修改文件权限/文件时间;
	支持OSS下载的content-type由文件后缀智能识别定义, 同时支持用户自定义扩展;
	支持Linux软链接文件;
	
###3、大文件支持
    单文件最大可以支持到OSS上限45TB;
    可支持数百万级别的文件管理;	

###4、已与多种流行应用集成
    Discuz/Wordpres/ownCloud/rsync/sftp/scp/apache/nginx/以及多种私有应用系统;

###5、稳定	
    已有数百位爱好者进行使用验证, 部分已经生产上线;

###6、操作简单, 可以天然无缝适配已有程序;	
	
###7、代码结构清晰, 可充分扩展;	

##欢迎使用

###用户支持交流群：307228533
