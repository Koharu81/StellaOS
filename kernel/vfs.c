/*
 * StellaOS - vfs.c
 * Virtual Filesystem layer and RAM filesystem (ramfs) implementation
 */

#include "types.h"
#include "vfs.h"
#include "memory.h"
#include "string.h"
#include "serial.h"

static vfs_node_t  inodes[MAX_INODES];
static vfs_mount_t mounts[MAX_MOUNT_POINTS];
static vfs_file_t  open_files[MAX_OPEN_FILES];
static vfs_dir_t   open_dirs[MAX_OPEN_DIRS];

static int allocate_inode(void) {
    for (int i = 0; i < MAX_INODES; i++) {
        if (!inodes[i].is_used) {
            inodes[i].is_used = true;
            inodes[i].inode_number = (uint32_t)i;
            inodes[i].data = NULL;
            inodes[i].size = 0;
            inodes[i].capacity = 0;
            inodes[i].permissions = 0755;
            inodes[i].parent_inode = 0;
            memset(inodes[i].name, 0, VFS_NAME_MAX);
            return i;
        }
    }
    return -1;
}

vfs_node_t *vfs_get_node(int inode_number) {
    if (inode_number >= 0 && inode_number < MAX_INODES && inodes[inode_number].is_used) {
        return &inodes[inode_number];
    }
    return NULL;
}

/* Helper to find a child node inside a given parent directory */
static vfs_node_t *find_child(int parent_inode, const char *name) {
    for (int i = 0; i < MAX_INODES; i++) {
        if (inodes[i].is_used && inodes[i].parent_inode == parent_inode) {
            if (i == 0 && parent_inode == 0) {
                continue; /* Skip root itself */
            }
            if (strcmp(inodes[i].name, name) == 0) {
                return &inodes[i];
            }
        }
    }
    return NULL;
}

vfs_node_t *vfs_resolve_path(const char *path) {
    if (!path || path[0] == '\0') {
        return NULL;
    }

    /* Root path "/" */
    if (path[0] == '/' && path[1] == '\0') {
        return &inodes[0];
    }

    int curr_inode = 0; /* Start at root directory */
    const char *p = path;
    if (*p == '/') {
        p++;
    }

    char component[VFS_NAME_MAX];

    while (*p != '\0') {
        /* Extract next path segment */
        size_t len = 0;
        while (*p != '/' && *p != '\0' && len < (VFS_NAME_MAX - 1)) {
            component[len++] = *p++;
        }
        component[len] = '\0';

        while (*p == '/') {
            p++; /* Skip repeated slashes */
        }

        if (len == 0 || strcmp(component, ".") == 0) {
            continue;
        }

        if (strcmp(component, "..") == 0) {
            curr_inode = inodes[curr_inode].parent_inode;
            continue;
        }

        vfs_node_t *child = find_child(curr_inode, component);
        if (!child) {
            return NULL; /* Path element not found */
        }

        curr_inode = (int)child->inode_number;
    }

    return &inodes[curr_inode];
}

/* Helper to separate path into parent path and leaf name */
static bool split_path(const char *path, char *parent_path, char *leaf_name) {
    if (!path || path[0] == '\0') {
        return false;
    }

    size_t len = strlen(path);
    /* Trim trailing slashes */
    while (len > 1 && path[len - 1] == '/') {
        len--;
    }

    int last_slash = -1;
    for (int i = (int)len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            last_slash = i;
            break;
        }
    }

    if (last_slash == -1) {
        /* No slash: relative to root */
        parent_path[0] = '/';
        parent_path[1] = '\0';
        strncpy(leaf_name, path, VFS_NAME_MAX - 1);
        leaf_name[VFS_NAME_MAX - 1] = '\0';
    } else if (last_slash == 0) {
        /* Directly under root */
        parent_path[0] = '/';
        parent_path[1] = '\0';
        size_t name_len = len - 1;
        if (name_len >= VFS_NAME_MAX) name_len = VFS_NAME_MAX - 1;
        memcpy(leaf_name, path + 1, name_len);
        leaf_name[name_len] = '\0';
    } else {
        size_t plen = (size_t)last_slash;
        if (plen >= VFS_NAME_MAX) plen = VFS_NAME_MAX - 1;
        memcpy(parent_path, path, plen);
        parent_path[plen] = '\0';

        size_t nlen = len - last_slash - 1;
        if (nlen >= VFS_NAME_MAX) nlen = VFS_NAME_MAX - 1;
        memcpy(leaf_name, path + last_slash + 1, nlen);
        leaf_name[nlen] = '\0';
    }

    return true;
}

int vfs_mkdir(const char *path) {
    if (!path || path[0] == '\0') {
        return -1;
    }

    /* Check if already exists */
    if (vfs_resolve_path(path) != NULL) {
        return -1;
    }

    char parent_path[VFS_NAME_MAX];
    char dir_name[VFS_NAME_MAX];
    if (!split_path(path, parent_path, dir_name)) {
        return -1;
    }

    vfs_node_t *parent_node = vfs_resolve_path(parent_path);
    if (!parent_node || parent_node->type != VFS_TYPE_DIR) {
        return -1;
    }

    int ino = allocate_inode();
    if (ino == -1) {
        return -1;
    }

    vfs_node_t *dir = &inodes[ino];
    strncpy(dir->name, dir_name, VFS_NAME_MAX - 1);
    dir->name[VFS_NAME_MAX - 1] = '\0';
    dir->type = VFS_TYPE_DIR;
    dir->size = 0;
    dir->permissions = 0755;
    dir->parent_inode = (int)parent_node->inode_number;
    dir->data = NULL;
    dir->capacity = 0;

    return 0;
}

/* Internal helper to create a file with initial data */
static int vfs_create_file(const char *path, const char *content) {
    char parent_path[VFS_NAME_MAX];
    char file_name[VFS_NAME_MAX];
    if (!split_path(path, parent_path, file_name)) {
        return -1;
    }

    vfs_node_t *parent_node = vfs_resolve_path(parent_path);
    if (!parent_node || parent_node->type != VFS_TYPE_DIR) {
        return -1;
    }

    int ino = allocate_inode();
    if (ino == -1) {
        return -1;
    }

    vfs_node_t *file = &inodes[ino];
    strncpy(file->name, file_name, VFS_NAME_MAX - 1);
    file->name[VFS_NAME_MAX - 1] = '\0';
    file->type = VFS_TYPE_FILE;
    file->permissions = 0644;
    file->parent_inode = (int)parent_node->inode_number;

    if (content) {
        size_t len = strlen(content);
        file->size = len;
        file->capacity = len + 16;
        file->data = kmalloc(file->capacity);
        if (file->data) {
            memcpy(file->data, content, len);
            ((char *)file->data)[len] = '\0';
        } else {
            file->size = 0;
            file->capacity = 0;
        }
    } else {
        file->size = 0;
        file->capacity = 64;
        file->data = kmalloc(file->capacity);
    }

    return ino;
}

int vfs_open(const char *path, int flags) {
    if (!path) {
        return -1;
    }

    vfs_node_t *node = vfs_resolve_path(path);
    if (!node) {
        if (flags & O_CREAT) {
            int ino = vfs_create_file(path, "");
            if (ino == -1) {
                return -1;
            }
            node = &inodes[ino];
        } else {
            return -1;
        }
    }

    if (node->type == VFS_TYPE_DIR) {
        return -1; /* Cannot open directory with file open */
    }

    /* Find free open file descriptor */
    int fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!open_files[i].is_open) {
            fd = i;
            break;
        }
    }

    if (fd == -1) {
        return -1;
    }

    if (flags & O_TRUNC) {
        node->size = 0;
    }

    open_files[fd].inode_number = (int)node->inode_number;
    open_files[fd].flags = flags;
    open_files[fd].offset = (flags & O_APPEND) ? node->size : 0;
    open_files[fd].is_open = true;

    return fd;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].is_open) {
        return -1;
    }

    open_files[fd].is_open = false;
    open_files[fd].inode_number = -1;
    open_files[fd].offset = 0;
    open_files[fd].flags = 0;

    return 0;
}

int vfs_read(int fd, void *buf, size_t count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].is_open || !buf) {
        return -1;
    }

    vfs_node_t *node = vfs_get_node(open_files[fd].inode_number);
    if (!node || node->type != VFS_TYPE_FILE) {
        return -1;
    }

    uint32_t offset = open_files[fd].offset;
    if (offset >= node->size) {
        return 0; /* EOF */
    }

    size_t to_read = count;
    if (offset + to_read > node->size) {
        to_read = node->size - offset;
    }

    if (node->data && to_read > 0) {
        memcpy(buf, (const uint8_t *)node->data + offset, to_read);
    }

    open_files[fd].offset += to_read;
    return (int)to_read;
}

int vfs_write(int fd, const void *buf, size_t count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].is_open || !buf) {
        return -1;
    }

    vfs_node_t *node = vfs_get_node(open_files[fd].inode_number);
    if (!node || node->type != VFS_TYPE_FILE) {
        return -1;
    }

    uint32_t offset = open_files[fd].offset;
    uint32_t required_size = offset + count;

    if (required_size > node->capacity) {
        uint32_t new_cap = node->capacity ? node->capacity * 2 : 128;
        if (new_cap < required_size + 64) {
            new_cap = required_size + 64;
        }

        void *new_data = kmalloc(new_cap);
        if (!new_data) {
            return -1;
        }

        if (node->data) {
            memcpy(new_data, node->data, node->size);
            kfree(node->data);
        }

        node->data = new_data;
        node->capacity = new_cap;
    }

    memcpy((uint8_t *)node->data + offset, buf, count);
    open_files[fd].offset += count;

    if (open_files[fd].offset > node->size) {
        node->size = open_files[fd].offset;
    }

    return (int)count;
}

int vfs_seek(int fd, int32_t offset, int whence) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !open_files[fd].is_open) {
        return -1;
    }

    vfs_node_t *node = vfs_get_node(open_files[fd].inode_number);
    if (!node) {
        return -1;
    }

    int32_t new_pos = 0;
    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = (int32_t)open_files[fd].offset + offset;
            break;
        case SEEK_END:
            new_pos = (int32_t)node->size + offset;
            break;
        default:
            return -1;
    }

    if (new_pos < 0) {
        new_pos = 0;
    }
    open_files[fd].offset = (uint32_t)new_pos;
    return new_pos;
}

int vfs_stat(const char *path, vfs_stat_t *st) {
    if (!path || !st) {
        return -1;
    }

    vfs_node_t *node = vfs_resolve_path(path);
    if (!node) {
        return -1;
    }

    st->st_ino = node->inode_number;
    st->st_type = node->type;
    st->st_size = node->size;
    st->st_mode = node->permissions;

    return 0;
}

int vfs_opendir(const char *path) {
    vfs_node_t *node = vfs_resolve_path(path);
    if (!node || node->type != VFS_TYPE_DIR) {
        return -1;
    }

    for (int i = 0; i < MAX_OPEN_DIRS; i++) {
        if (!open_dirs[i].is_open) {
            open_dirs[i].inode_number = (int)node->inode_number;
            open_dirs[i].child_index = 0;
            open_dirs[i].is_open = true;
            return i;
        }
    }

    return -1;
}

int vfs_readdir(int dfd, vfs_dirent_t *dirent) {
    if (dfd < 0 || dfd >= MAX_OPEN_DIRS || !open_dirs[dfd].is_open || !dirent) {
        return 0;
    }

    int target_parent = open_dirs[dfd].inode_number;
    uint32_t *index = &open_dirs[dfd].child_index;

    for (uint32_t i = *index; i < MAX_INODES; i++) {
        if (inodes[i].is_used && inodes[i].parent_inode == target_parent) {
            /* Skip root node if directory is root */
            if (i == 0 && target_parent == 0) {
                continue;
            }

            strncpy(dirent->d_name, inodes[i].name, VFS_NAME_MAX - 1);
            dirent->d_name[VFS_NAME_MAX - 1] = '\0';
            dirent->d_ino = inodes[i].inode_number;
            dirent->d_type = inodes[i].type;
            dirent->d_size = inodes[i].size;

            *index = i + 1;
            return 1;
        }
    }

    *index = MAX_INODES;
    return 0;
}

int vfs_closedir(int dfd) {
    if (dfd < 0 || dfd >= MAX_OPEN_DIRS || !open_dirs[dfd].is_open) {
        return -1;
    }

    open_dirs[dfd].is_open = false;
    open_dirs[dfd].inode_number = -1;
    open_dirs[dfd].child_index = 0;

    return 0;
}

void vfs_init(void) {
    memset(inodes, 0, sizeof(inodes));
    memset(mounts, 0, sizeof(mounts));
    memset(open_files, 0, sizeof(open_files));
    memset(open_dirs, 0, sizeof(open_dirs));

    /* Initialize root directory "/" at inode 0 */
    inodes[0].is_used = true;
    inodes[0].inode_number = 0;
    strncpy(inodes[0].name, "/", VFS_NAME_MAX - 1);
    inodes[0].type = VFS_TYPE_DIR;
    inodes[0].size = 0;
    inodes[0].permissions = 0755;
    inodes[0].parent_inode = 0;
    inodes[0].data = NULL;
    inodes[0].capacity = 0;

    /* Register root mount point */
    mounts[0].is_mounted = true;
    strncpy(mounts[0].path, "/", VFS_NAME_MAX - 1);
    strncpy(mounts[0].name, "ramfs", VFS_NAME_MAX - 1);
    mounts[0].root_inode = 0;

    /* Create initial directory structure */
    vfs_mkdir("/home");
    vfs_mkdir("/home/stella");
    vfs_mkdir("/etc");
    vfs_mkdir("/tmp");
    vfs_mkdir("/var");
    vfs_mkdir("/usr");
    vfs_mkdir("/usr/share");
    vfs_mkdir("/usr/share/wallpapers");
    vfs_mkdir("/sys");
    vfs_mkdir("/dev");

    /* Create sample initial files */
    vfs_create_file("/home/stella/welcome.txt",
                    "Welcome to StellaOS!\n"
                    "A lightweight kawaii x86 operating system.\n"
                    "Enjoy exploring the desktop, applications, and core systems!\n");

    vfs_create_file("/etc/hostname", "stella-pc\n");

    vfs_create_file("/etc/os-release",
                    "NAME=\"StellaOS\"\n"
                    "VERSION=\"1.0\"\n"
                    "ID=stella\n"
                    "PRETTY_NAME=\"StellaOS 1.0 (Anime Kawaii Edition)\"\n"
                    "HOME_URL=\"https://stella-os.org\"\n");

    serial_printf("VFS: Initialized ramfs root filesystem and directory hierarchy\n");
}
