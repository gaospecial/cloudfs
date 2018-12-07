#!/bin/bash

#./CloudFS_Install.sh INSTALL_DIR=XXXX MOUNT_POINT=XXXX

BASE_PATH=$(cd `dirname $0`; pwd)
#echo $BASE_PATH

PARAM1=$1
PARAM2=$2

INSTALL_DIR=
MOUNT_POINT=

DEFAULT_INSTALL_DIR="/usr/local/cloudfs"
DEFAULT_MOUNT_POINT="/mnt/oss"

function fParamProc()
{
	#echo $1
	PARAM=$1
	LETF=$(echo $PARAM | awk -F"=" '{print $1}')
	#echo $LETF
	RIGHT=$(echo $PARAM | awk -F"=" '{print $2}')
	#echo $RIGHT

	if [ $LETF = "INSTALL_DIR" ]; then
		INSTALL_DIR=$RIGHT
	elif [ $LETF = "MOUNT_POINT" ]; then
		MOUNT_POINT=$RIGHT
	else 
		echo "unknown name."
	fi
}

if [ "x$PARAM1" != "x" ]; then
	fParamProc $PARAM1
fi

if [ "x$PARAM2" != "x" ]; then
	fParamProc $PARAM2
fi

if [ "x$INSTALL_DIR" = "x" ]; then
	INSTALL_DIR=$DEFAULT_INSTALL_DIR
fi

if [ "x$MOUNT_POINT" = "x" ]; then
	MOUNT_POINT=$DEFAULT_MOUNT_POINT
fi

LINUX_DISTRIBUTION=$(head -1 /etc/issue | cut -b 1-6)
if [ $LINUX_DISTRIBUTION = "Ubuntu" ]; then
	cp $BASE_PATH/conf/cloudfs.ubuntu.autoboot $BASE_PATH/conf/cloudfs.init.conf
else
	cp $BASE_PATH/conf/cloudfs.centos.autoboot $BASE_PATH/conf/cloudfs.init.conf
fi
REBOOT_SCRPTS_NAME="$BASE_PATH/conf/cloudfs.init.conf"


if [ $LINUX_DISTRIBUTION = "Ubuntu" ]; then
	INSTALL_DIR_POS=$(cat -n $REBOOT_SCRPTS_NAME | grep "env BIN_PATH=" | awk '{print $1}')
	sed -i $INSTALL_DIR_POS'c \env BIN_PATH=\"'$INSTALL_DIR'\"' $REBOOT_SCRPTS_NAME

	MOUNT_POINT_POS=$(cat -n $REBOOT_SCRPTS_NAME | grep "env MOUNT_POINT=" | awk '{print $1}')
	sed -i $MOUNT_POINT_POS'c \env MOUNT_POINT=\"'$MOUNT_POINT'\"'  $REBOOT_SCRPTS_NAME
else
	INSTALL_DIR_POS=$(cat -n $REBOOT_SCRPTS_NAME | grep "BIN_PATH=" | awk '{print $1}')
        sed -i $INSTALL_DIR_POS'c \BIN_PATH=\"'$INSTALL_DIR'\"' $REBOOT_SCRPTS_NAME

        MOUNT_POINT_POS=$(cat -n $REBOOT_SCRPTS_NAME | grep "MOUNT_POINT=" | awk '{print $1}')
        sed -i $MOUNT_POINT_POS'c \MOUNT_POINT=\"'$MOUNT_POINT'\"'  $REBOOT_SCRPTS_NAME
fi

if [ $LINUX_DISTRIBUTION = "Ubuntu" ]; then
	mv $REBOOT_SCRPTS_NAME /etc/init/cloudfs.conf
else
	mv $REBOOT_SCRPTS_NAME /etc/init.d/cloudfs
	chmod 755 /etc/init.d/cloudfs
	cd /etc/init.d/
	chkconfig --add cloudfs
fi

if [ ! -d $INSTALL_DIR ]; then 
	mkdir -p $INSTALL_DIR
fi

cp $BASE_PATH/cloudfs $INSTALL_DIR
chmod +x $INSTALL_DIR/cloudfs

if [ ! -d $INSTALL_DIR/conf ]; then
	mkdir -p $INSTALL_DIR/conf
fi

cp $BASE_PATH/conf/cloudfs.conf $INSTALL_DIR/conf
cp $BASE_PATH/conf/header_params $INSTALL_DIR/conf
cp $BASE_PATH/conf/mime.types $INSTALL_DIR/conf

echo " * Install CloudFS success......."
exit 0
# replace the reboot scrpts

