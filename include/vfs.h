/*
 * StellaOS - vfs.h
 * Virtual Filesystem layer interface
 */

#ifndef STELLA_VFS_H
#define STELLA_VFS_H

#include "types.h"

#define MAX_MOUNT_POINTS 16
#define MAX_INODES       256
#define MAX_OPEN_FILES   32
#define MAX_OPEN_DIRS    16
#define VFS_NAME_MAX     64

/* VFS node types */
#define VFS_TYPE_NONE    0
#define VFS_TYPE_FILE    1
#define VFS_TYPE_DIR     2
#define VFS_TYPE_DEVICE  3

/* Open flags */
#define O_RDONLY         0x0001
#define O_WRONLY         0x0002
#define O_RDWR           0x0003
#define O_CREAT          0x0040
#define O_TRUNC          0x0200
#define O_APPEND         0x0400

/* Seek whence */
#define SEEK_SET         0
#define SEEK_CUR         1
#define SEEK_END         2

/* Stat structure */
typedef struct vfs_stat {
    uint32_t st_ino;
    uint32_t st_type;
    uint32_t st_size;
    uint32_t st_mode;
} vfs_stat_t;

/* Directory entry structure */
typedef struct vfs_dirent {
    char d_name[VFS_NAME_MAX];
    uint32_t d_ino;
    uint32_t d_type;
    uint32_t d_size;
} vfs_dirent_t;

/* VFS Node structure */
typedef struct vfs_node {
    char name[VFS_NAME_MAX];
    uint32_t type;
    uint32_t size;
    uint32_t permissions;
    uint32_t inode_number;
    void *data;                 /* Dynamic data buffer for ramfs */
    uint32_t capacity;          /* Allocated capacity for data buffer */
    int parent_inode;           /* Inode number of parent directory */
    bool is_used;
} vfs_node_t;

/* Mount point structure */
typedef struct vfs_mount {
    char path[VFS_NAME_MAX];
    char name[VFS_NAME_MAX];
    int root_inode;
    bool is_mounted;
} vfs_mount_t;

/* File descriptor entry */
typedef struct vfs_file {
    int inode_number;
    uint32_t offset;
    int flags;
    bool is_open;
} vfs_file_t;

/* Directory stream entry */
typedef struct vfs_dir {
    int inode_number;
    uint32_t child_index;       /* Current enumeration index */
    bool is_open;
} vfs_dir_t;

/* VFS Public API */
void vfs_init(void);

/* File operations */
int vfs_open(const char *path, int flags);
int vfs_close(int fd);
int vfs_read(int fd, void *buf, size_t count);
int vfs_write(int fd, const void *buf, size_t count);
int vfs_seek(int fd, int32_t offset, int whence);
int vfs_stat(const char *path, vfs_stat_t *st);

/* Directory operations */
int vfs_mkdir(const char *path);
int vfs_opendir(const char *path);
int vfs_readdir(int dfd, vfs_dirent_t *dirent);
int vfs_closedir(int dfd);

/* Utility lookups */
vfs_node_t *vfs_resolve_path(const char *path);
vfs_node_t *vfs_get_node(int inode_number);

#endif /* STELLA_VFS_H */
