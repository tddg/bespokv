//#include "cb.hpp"

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <iostream>
#include <stdlib.h>

/* get attribute */
int confs_getattr(const char* path, struct stat* stbuf);
int confs_open(const char* path, struct fuse_file_info* fi);
int confs_mknod(const char *path, mode_t mode, dev_t dev);
int confs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi);
int confs_truncate(const char* path, off_t size);
int confs_read(const char* path, char *buf, size_t size, off_t offset, struct fuse_file_info* fi);
int confs_write(const char* path, const char *buf, size_t size, off_t offset, struct fuse_file_info* fi);
int confs_rename(const char* from, const char* to);
int confs_unlink(const char* path);

void confs_fullpath(char fpath[PATH_MAX], const char *path);
void *confs_init(struct fuse_conn_info *conn);
void confs_usage();

#define CONFS_DATA ((struct confs_state *) fuse_get_context()->private_data)
