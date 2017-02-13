/*
    FUSE: Filesystem in Userspace
*/

#include "confs.hpp"

struct fuse_operations confs_op;

int confs_getattr(const char* path, struct stat* stbuf){

	char fpath[PATH_MAX];
	confs_fullpath(fpath, path);

    int res;

    res = lstat(fpath, stbuf);

    fprintf(stderr, "Inside getattr\n");

    if (res == -1)
        return -errno;

    return 0;
}

int confs_open(const char* path, struct fuse_file_info* fi){
    int res;
	char fpath[PATH_MAX];
	confs_fullpath(fpath, path);

    res = open(fpath, fi->flags);
    if (res == -1)
        return -errno;

    fprintf(stderr, "Inside open\n");

    close(res);
    return 0;
}

int confs_mknod(const char *path, mode_t mode, dev_t dev){
    int res;
	char fpath[PATH_MAX];
	confs_fullpath(fpath, path);

    /* On Linux this could just be 'mknod(path, mode, dev)' but this
       is more portable */
    if (S_ISREG(mode)) {
        res = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
        if (res >= 0)
            res = close(res);
    } else if (S_ISFIFO(mode))
        res = mkfifo(fpath, mode);
    else
        res = mknod(fpath, mode, dev);
    if (res == -1)
        return -errno;

    return 0;
}

int confs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi){
    DIR *dp;
    struct dirent *de;

    (void) offset;
    (void) fi;
	char fpath[PATH_MAX];
	confs_fullpath(fpath, path);

    dp = opendir(fpath);
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

int confs_truncate(const char* path, off_t size){
    int res;
	char fpath[PATH_MAX];
	confs_fullpath(fpath, path);

    res = truncate(fpath, size);
    if (res == -1)
        return -errno;

    return 0;
}

int confs_read(const char* path, char *buf, size_t size, off_t offset, struct fuse_file_info* fi){
    int fd;
    int res;
	char fpath[PATH_MAX];
	confs_fullpath(fpath, path);

    (void) fi;
    fd = open(fpath, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    fprintf(stderr, "Inside read\n");

    close(fd);
    return res;
}

int confs_write(const char* path, const char *buf, size_t size, off_t offset, struct fuse_file_info* fi){
    int fd;
    int res;
	char fpath[PATH_MAX];
	confs_fullpath(fpath, path);

    (void) fi;
    fd = open(fpath, O_WRONLY);
    if (fd == -1)
        return -errno;

    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

int confs_rename(const char* from, const char* to){
    int res;
	char fpathfrom[PATH_MAX];
	confs_fullpath(fpathfrom, from);

	char fpathto[PATH_MAX];
	confs_fullpath(fpathto, to);


    res = rename(fpathfrom, fpathto);
    if (res == -1)
        return -errno;

    return 0;
}

int confs_unlink(const char* path){
    int res;
	char fpath[PATH_MAX];
	confs_fullpath(fpath, path);

    res = unlink(fpath);
    if (res == -1)
        return -errno;

    return 0;
}

/* inisialisasi mapping fungsi-fungsi fuse */
void init_fuse(){
	confs_op.init		= confs_init;
	confs_op.getattr	= confs_getattr;
	confs_op.open		= confs_open;
	confs_op.read		= confs_read;
	confs_op.write		= confs_write;
	confs_op.truncate	= confs_truncate;
	confs_op.rename		= confs_rename;
	confs_op.readdir	= confs_readdir;
	confs_op.mknod		= confs_mknod;
	confs_op.unlink		= confs_unlink;
}

using namespace std;


struct confs_state {
    char *rootdir;
};

void *confs_init(struct fuse_conn_info *conn)
{
    return CONFS_DATA;
}

void confs_fullpath(char fpath[PATH_MAX], const char *path)
{
    strcpy(fpath, CONFS_DATA->rootdir);
    strncat(fpath, path, PATH_MAX); // ridiculously long paths will
				    // break here
}

void confs_usage()
{
    fprintf(stderr, "usage:  confs [FUSE and mount options] mountPoint rootDir\n");
    abort();
}

int main(int argc, char** argv){

    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
    	confs_usage();

    umask(0);
    init_fuse();
    struct confs_state *confs_data = new(confs_state);
    //confs_data = malloc(sizeof(struct confs_state));
    char* fuse_argv[2] = {argv[0], argv[1]};
    confs_data->rootdir = realpath(argv[2], NULL);
    argv[argc-1] = NULL;
    argc--;

    return fuse_main(argc, argv, &confs_op, confs_data);

    /**
	int fuse_argc = 2;
	char* fuse_argv[2] = {argv[0], argv[1]};
	return fuse_main(fuse_argc, fuse_argv, &confs_op, NULL);
	**/
}
