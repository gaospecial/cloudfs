

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include "Wrapper.h"
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

using namespace std;
//using namespace afs;
static struct fuse_operations g_afs_opers;
extern OssFS *g_fs;
pid_t g_pid;				// 获取当前进程PID

int main(int argc, char *argv[]) 
{
	g_pid = getpid();

    g_fs=new OssFS();
    g_afs_opers.getattr =ali_getattr;
    g_afs_opers.readlink = ali_readlink;
    g_afs_opers.getdir = NULL;
    g_afs_opers.mkdir = ali_mkdir;
    g_afs_opers.unlink = ali_unlink;
    g_afs_opers.rmdir = ali_rmdir;
    g_afs_opers.symlink = ali_symlink;
    g_afs_opers.rename = ali_rename;
    g_afs_opers.link = ali_link;
    g_afs_opers.chmod = ali_chmod;
    g_afs_opers.chown = ali_chown;
    g_afs_opers.truncate = ali_truncate;
    g_afs_opers.utime = ali_utime;
    g_afs_opers.open = ali_open;
    g_afs_opers.create=ali_create;
    g_afs_opers.read = ali_read;
    g_afs_opers.write = ali_write;
    g_afs_opers.statfs = ali_statfs;
    g_afs_opers.flush = ali_flush;
    g_afs_opers.release = ali_release;
    g_afs_opers.fsync = ali_fsync;
    g_afs_opers.readdir = ali_readdir;
    g_afs_opers.fsyncdir = ali_fsyncdir;
    g_afs_opers.init = ali_init;
    g_afs_opers.destroy = ali_destroy;
	g_afs_opers.access = ali_access;
//	set_log_level(ERROR);
    return fuse_main(argc, argv, &g_afs_opers, NULL);
}
