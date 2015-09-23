

#ifndef WRAPPER_H_
#define WRAPPER_H_
#include <fuse.h>
#include "OssFS.h"
//using namespace afs;
OssFS *g_fs = NULL;//please find some place to delete
int ali_getattr(const char *path, struct stat *statbuf) {
    return g_fs->getattr(path, statbuf);
}
int ali_readlink(const char *path, char *link, size_t size) {
    return g_fs->readlink(path, link, size);
}
int ali_mkdir(const char *path, mode_t mode) {
    return g_fs->mkdir(path, mode);
}
int ali_unlink(const char *path) {
    return g_fs->unlink(path);
}
int ali_rmdir(const char *path) {
    return g_fs->rmdir(path);
}
int ali_symlink(const char *path, const char *link) {
    return g_fs->symlink(path, link);
}
int ali_rename(const char *path, const char *newpath) {
    return g_fs->rename(path, newpath);
}
int ali_link(const char *path, const char *newpath) {
    return g_fs->link(path, newpath);
}
int ali_chmod(const char *path, mode_t mode) {
    return g_fs->chmod(path, mode);
}
int ali_chown(const char *path, uid_t uid, gid_t gid) {
    return g_fs->chown(path, uid, gid);
}
int ali_truncate(const char *path, off_t newSize) {
    return g_fs->truncate(path, newSize);
}
int ali_utime(const char *path, struct utimbuf *ubuf) {
    return g_fs->utime(path, ubuf);
}
int ali_open(const char *path, struct fuse_file_info *fileInfo) {
    return g_fs->open(path, fileInfo);
}
int ali_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    return g_fs->create(path, mode, fi);
}
int ali_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fileInfo) {
    return g_fs->read(path, buf, size, offset, fileInfo);
}
int ali_write(const char *path, const char *buf, size_t size, off_t offset,
        struct fuse_file_info *fileInfo) {
    return g_fs->write(path, buf, size, offset, fileInfo);
}
int ali_statfs(const char *path, struct statvfs *statInfo) {
    return g_fs->statfs(path, statInfo);
}
int ali_flush(const char *path, struct fuse_file_info *fileInfo) {
    return g_fs->flush(path, fileInfo);
}
int ali_release(const char *path, struct fuse_file_info *fileInfo) {
    return g_fs->release(path, fileInfo);
}
int ali_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
    return g_fs->fsync(path, datasync, fi);
}
int ali_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fileInfo) {
    return g_fs->readdir(path, buf, filler, offset, fileInfo);
}
int ali_fsyncdir(const char *path, int datasync,
        struct fuse_file_info *fileInfo) {
    return g_fs->fsyncdir(path, datasync, fileInfo);
}

int ali_access(const char *path, int mode){
	return g_fs->access(path,mode);
}

void *ali_init(struct fuse_conn_info* conn) {
    g_fs->init(conn);
    return 0;
}
void ali_destroy(void *p) {
    g_fs->destroy();
    delete g_fs;
}
#endif /* WRAPPER_H_ */
