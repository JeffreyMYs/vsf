/*****************************************************************************
 *   Copyright(C)2009-2022 by VSF Team                                       *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 ****************************************************************************/

/*============================ INCLUDES ======================================*/

#include "../../vsf_fs_cfg.h"

#if VSF_USE_FS == ENABLED && VSF_FS_USE_LINFS == ENABLED

#define __VSF_FS_CLASS_INHERIT__
#define __VSF_LINFS_CLASS_IMPLEMENT
#define __VSF_HEADER_ONLY_SHOW_FS_INFO__
#include "../../vsf_fs.h"
#include "./vsf_linfs.h"

#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

/*============================ MACROS ========================================*/
/*============================ MACROFIED FUNCTIONS ===========================*/
/*============================ TYPES =========================================*/
/*============================ PROTOTYPES ====================================*/

dcl_vsf_peda_methods(static, __vk_linfs_mount)
dcl_vsf_peda_methods(static, __vk_linfs_lookup)
dcl_vsf_peda_methods(static, __vk_linfs_read)
dcl_vsf_peda_methods(static, __vk_linfs_write)
dcl_vsf_peda_methods(static, __vk_linfs_close)

extern vk_file_t * __vk_file_get_fs_parent(vk_file_t *file);

/*============================ GLOBAL VARIABLES ==============================*/

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

const vk_fs_op_t vk_linfs_op = {
    .fn_mount       = (vsf_peda_evthandler_t)vsf_peda_func(__vk_linfs_mount),
    .fn_unmount     = (vsf_peda_evthandler_t)vsf_peda_func(vk_dummyfs_succeed),
#if VSF_FS_CFG_USE_CACHE == ENABLED
    .fn_sync        = vk_file_dummy,
#endif
    .fop            = {
        .fn_read    = (vsf_peda_evthandler_t)vsf_peda_func(__vk_linfs_read),
        .fn_write   = (vsf_peda_evthandler_t)vsf_peda_func(__vk_linfs_write),
        .fn_close   = (vsf_peda_evthandler_t)vsf_peda_func(__vk_linfs_close),
        .fn_resize  = (vsf_peda_evthandler_t)vsf_peda_func(vk_dummyfs_not_support),
    },
    .dop            = {
        .fn_lookup  = (vsf_peda_evthandler_t)vsf_peda_func(__vk_linfs_lookup),
        .fn_create  = (vsf_peda_evthandler_t)vsf_peda_func(vk_dummyfs_not_support),
        .fn_unlink  = (vsf_peda_evthandler_t)vsf_peda_func(vk_dummyfs_not_support),
        .fn_chmod   = (vsf_peda_evthandler_t)vsf_peda_func(vk_dummyfs_not_support),
        .fn_rename  = (vsf_peda_evthandler_t)vsf_peda_func(vk_dummyfs_not_support),
    },
};

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic pop
#endif

/*============================ LOCAL VARIABLES ===============================*/
/*============================ IMPLEMENTATION ================================*/

static uint_fast16_t __vk_linfs_file_get_path(vk_file_t *file, char *path, uint_fast16_t len)
{
    vk_file_t *tmp = file;
    uint_fast16_t real_len = 0, cur_len;

    while (tmp != NULL) {
        if (&vk_vfs_op == tmp->fsop) {
            tmp = ((vk_vfs_file_t *)tmp)->subfs.root;
        }
        real_len += strlen(tmp->name) + 1;
        tmp = tmp->parent;
    }

    if (real_len > len) {
        return 0;
    }

    tmp = file;
    len = real_len - 1;
    path[len] = '\0';
    while (tmp != NULL) {
        if (&vk_vfs_op == tmp->fsop) {
            tmp = ((vk_vfs_file_t *)tmp)->subfs.root;
        }
        cur_len = strlen(tmp->name);
        len -= cur_len;
        memcpy(&path[len], tmp->name, cur_len);
        tmp = tmp->parent;
        if (tmp != NULL) {
            path[len-- - 1] = '/';
        }
    }
    return real_len;
}

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wcast-align"
#elif   __IS_COMPILER_LLVM__ || __IS_COMPILER_ARM_COMPILER_6__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wcast-align"
#endif

__vsf_component_peda_ifs_entry(__vk_linfs_mount, vk_fs_mount)
{
    vsf_peda_begin();
    vk_vfs_file_t *dir = (vk_vfs_file_t *)&vsf_this;
    vk_linfs_info_t *fsinfo = dir->subfs.data;
    VSF_FS_ASSERT((fsinfo != NULL) && (fsinfo->root.name != NULL));

    struct stat statbuf;
    if (0 != stat(fsinfo->root.name, &statbuf)) {
        vsf_eda_return(VSF_ERR_NOT_AVAILABLE);
        return;
    }
    if (!S_ISDIR(statbuf.st_mode)) {
        vsf_eda_return(VSF_ERR_INVALID_PARAMETER);
        return;
    }

    dir->subfs.root = &fsinfo->root.use_as__vk_file_t;
    vsf_eda_return(VSF_ERR_NONE);
    vsf_peda_end();
}

__vsf_component_peda_ifs_entry(__vk_linfs_lookup, vk_file_lookup)
{
    vsf_peda_begin();
    vk_linfs_file_t *dir = (vk_linfs_file_t *)&vsf_this;
    const char *name = vsf_local.name;
    uint_fast32_t idx = vsf_local.idx;
    vsf_err_t err = VSF_ERR_NONE;

    vsf_protect_t orig = vsf_protect_sched();
        __vsf_dlist_foreach_unsafe(vk_linfs_file_t, child_node, &dir->d.child_list) {
            if (    (name && vk_file_is_match((char *)name, _->name))
                ||  (!name && (_->idx == idx))) {
                vsf_unprotect_sched(orig);
                *vsf_local.result = &_->use_as__vk_file_t;
                goto do_return;
            }
        }
    vsf_unprotect_sched(orig);

    char path[PATH_MAX];
    uint_fast16_t len = __vk_linfs_file_get_path(&dir->use_as__vk_file_t, path, sizeof(path));
    uint_fast16_t namelen;

    *vsf_local.result = NULL;
    if (name != NULL) {
        const char *ptr = name;
        while (*ptr != '\0') {
            if (vk_file_is_div(*ptr)) {
                break;
            }
            ptr++;
        }
        if ('\0' == *ptr) {
            namelen = strlen(name);
        } else {
            namelen = ptr - name;
        }
        if ((len + namelen + 1) > PATH_MAX) {
            err = VSF_ERR_FAIL;
            goto do_return;
        }
        path[len - 1] = '/';
        memcpy(&path[len], name, namelen);
        path[len + namelen] = '\0';
    } else {
        DIR *dir = opendir(path);
        if (dir != NULL) {
            closedir(dir);

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (    !strcmp(entry->d_name, ".")
                    ||  !strcmp(entry->d_name, "..")) {
                    idx++;
                }
                if (!idx) {
                    break;
                }
            }

            namelen = strlen(entry->d_name);
            if ((len + namelen + 1) > PATH_MAX) {
                err = VSF_ERR_FAIL;
                goto do_return;
            }
            path[len] = '\0';
            strcat(path, entry->d_name);
        } else {
            goto do_not_available;
        }
    }

    struct stat statbuf;
    if (0 != stat(path, &statbuf)) {
    do_not_available:
        err = VSF_ERR_NOT_AVAILABLE;
        goto do_return;
    }

    vk_linfs_file_t *linfs_file = (vk_linfs_file_t *)vk_file_alloc(sizeof(*linfs_file));
    if (NULL == linfs_file) {
        err = VSF_ERR_NOT_ENOUGH_RESOURCES;
        goto do_return;
    }
    linfs_file->name = vsf_heap_malloc(namelen + 1);
    if (NULL == linfs_file->name) {
        err = VSF_ERR_NOT_ENOUGH_RESOURCES;
        goto do_free_and_return;
    }
    strcpy(linfs_file->name, vk_file_getfilename(path));
    linfs_file->fsop = &vk_linfs_op;

    if (S_ISDIR(statbuf.st_mode)) {
        linfs_file->attr |= VSF_FILE_ATTR_DIRECTORY;
    } else {
        linfs_file->f.fd = open(path, 0);
        if (-1 == linfs_file->f.fd) {
            err = VSF_ERR_NOT_AVAILABLE;
            goto do_free_and_return;
        }
        linfs_file->size = lseek(linfs_file->f.fd, 0, SEEK_END);
        lseek(linfs_file->f.fd, 0, SEEK_SET);
    }
    linfs_file->attr |= VSF_FILE_ATTR_READ | VSF_FILE_ATTR_WRITE;

    orig = vsf_protect_sched();
        vsf_dlist_add_to_head(vk_linfs_file_t, child_node, &dir->d.child_list, linfs_file);
    vsf_unprotect_sched(orig);
    *vsf_local.result = &linfs_file->use_as__vk_file_t;
    goto do_return;

do_free_and_return:
    if (linfs_file->name != NULL) {
        vsf_heap_free(linfs_file->name);
    }
    vk_file_free(&linfs_file->use_as__vk_file_t);
do_return:
    vsf_eda_return(err);
    vsf_peda_end();
}

__vsf_component_peda_ifs_entry(__vk_linfs_read, vk_file_read)
{
    vsf_peda_begin();
    vk_linfs_file_t *file = (vk_linfs_file_t *)&vsf_this;

    if (-1 != lseek(file->f.fd, vsf_local.offset, SEEK_SET)) {
        vsf_eda_return(read(file->f.fd, vsf_local.buff, vsf_local.size));
    } else {
        vsf_eda_return(VSF_ERR_FAIL);
    }
    vsf_peda_end();
}

__vsf_component_peda_ifs_entry(__vk_linfs_write, vk_file_write)
{
    vsf_peda_begin();
    vk_linfs_file_t *file = (vk_linfs_file_t *)&vsf_this;

    if (-1 != lseek(file->f.fd, vsf_local.offset, SEEK_SET)) {
        vsf_eda_return(write(file->f.fd, vsf_local.buff, vsf_local.size));
    } else {
        vsf_eda_return(VSF_ERR_FAIL);
    }
    vsf_peda_end();
}

__vsf_component_peda_ifs_entry(__vk_linfs_close, vk_file_close)
{
    vsf_peda_begin();
    vk_linfs_file_t *file = (vk_linfs_file_t *)&vsf_this;
    vk_linfs_file_t *parent = (vk_linfs_file_t *)__vk_file_get_fs_parent(&file->use_as__vk_file_t);

    VSF_FS_ASSERT(file->name != NULL);
    vsf_heap_free(file->name);

    if (file->attr & VSF_FILE_ATTR_DIRECTORY) {
        // TODO: Close Directory
    } else {
        close(file->f.fd);
    }
    vsf_protect_t orig = vsf_protect_sched();
        vsf_dlist_remove(vk_linfs_file_t, child_node, &parent->d.child_list, file);
    vsf_unprotect_sched(orig);
    vsf_eda_return(VSF_ERR_NONE);
    vsf_peda_end();
}

#if     __IS_COMPILER_GCC__
#   pragma GCC diagnostic pop
#elif   __IS_COMPILER_LLVM__ || __IS_COMPILER_ARM_COMPILER_6__
#   pragma clang diagnostic pop
#endif

#endif