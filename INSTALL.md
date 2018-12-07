# 在 Ubuntu 上安装

如果是 Ubuntu 14.04，那么可以使用最初自动化安装脚本。


我在 Ubuntu 16.04 上面，按照下列命令安装的。

```
# 编译
git clone https://github.com/gaospecial/cloudfs
cd cloudfs/src
make

# 复制到系统文件夹
mkdir -p /usr/local/cloudfs
mv cloudfs /usr/local/cloudfs/
mv conf /usr/local/cloudfs/
```

## 修改配置文件

配置文件在 `./conf/cloudfs.conf`。

需要设置四个参数，分别是`HOST`，`ID`，`KEY`和`BUCKET`。这些信息都在阿里云的控制面板里面可以查得到。

```
HOST=oss-cn-qingdao-internal.aliyuncs.com
#HOST=oss-cn-beijing-internal.aliyuncs.com
#HOST=oss-cn-shenzhen-internal.aliyuncs.com
#HOST=oss-cn-hangzhou-internal.aliyuncs.com
#HOST=oss-cn-hongkong-internal.aliyuncs.com
# NOT ECS, but user's server
#HOST=oss-cn-qingdao.aliyuncs.com
#HOST=oss-cn-beijing.aliyuncs.com
#HOST=oss-cn-shenzhen.aliyuncs.com
#HOST=oss-cn-hangzhou.aliyuncs.com
#HOST=oss-cn-hongkong.aliyuncs.com

# OSS Access Id and Access Key configuration, uncommet it and change the value
# to your own id/key.
ID=your_access_id
KEY=your_access_key

# OSS bucket configuraion, uncommet it and change the value to you own bucket name
BUCKET=your_bucket_name
```

## 设置自启动

自启动文件在 `./conf/cloudfs.service`。
关于配置的[说明](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/system_administrators_guide/sect-managing_services_with_systemd-unit_files)。

```
[Unit]
Description=cloudfs service

[Service]
Type=oneshot
WorkingDirectory=/usr/local/cloudfs
ExecStartPre=-/bin/fusermount -uq /mnt/oss
ExecStart=/usr/local/cloudfs/cloudfs /mnt/oss -o allow_other
RemainAfterExit=True
ExecStop=/usr/bin/killall cloudfs
ExecStop=/bin/fusermount -uq /mnt/oss

[Install]
WantedBy=multi-user.target
```

如果安装路径或挂载点有变化，需要相应予以改动。

将该文件保存到 `/etc/systemd/system` 文件夹。

```
cp ./conf/cloudfs.service /etc/systemd/system

# 运行cloudfs
systemctl start cloudfs

# 终止cloudfs
systemctl stop cloudfs

# 加入开机自启动
systemctl enable cloudfs

# 去掉开机自启动
systemct disable cloudfs
```

运行并加入自启动就可以一劳永逸了。