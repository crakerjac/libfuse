/*
    FUSE: Filesystem in Userspace
    Copyright (C) 2001  Miklos Szeredi (mszeredi@inf.bme.hu)

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.
*/

#include "fuse.h"
#include "fuse_i.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/smp_lock.h>
#include <linux/sched.h>
#include <linux/file.h>

#define FUSE_SUPER_MAGIC 0x65735546

static void fuse_read_inode(struct inode *inode)
{
	printk(KERN_DEBUG "fuse_read_inode: %li\n", inode->i_ino);

	
}

static void fuse_put_super(struct super_block *sb)
{
	struct fuse_conn *fc = sb->u.generic_sbp;

	printk(KERN_DEBUG "fuse_put_super\n");

	fc->sb = NULL;
	fuse_release_conn(fc);
}

static struct super_operations fuse_super_operations = {
	read_inode:	fuse_read_inode,
	put_super:	fuse_put_super,
};


static struct fuse_conn *get_conn(struct fuse_mount_data *d)
{
	struct fuse_conn *fc = NULL;
	struct file *file;
	struct inode *ino;

	printk(KERN_DEBUG "fuse_read_super\n");

	if(d == NULL) {
		printk("fuse_read_super: Bad mount data\n");
		return NULL;
	}

	if(d->version != FUSE_MOUNT_VERSION) {
		printk("fuse_read_super: Bad mount version: %i\n", d->version);
		return NULL;
	}

	file = fget(d->fd);
	ino = NULL;
	if(file)
		ino = file->f_dentry->d_inode;
	
	if(!ino || ino->u.generic_ip != proc_fuse_dev) {
		printk("fuse_read_super: Bad file: %i\n", d->fd);
		goto out;
	}

	fc = file->private_data;

  out:
	fput(file);
	return fc;

}

static struct super_block *fuse_read_super(struct super_block *sb, 
					   void *data, int silent)
{	
	struct fuse_conn *fc;
	struct inode *root;

	fc = get_conn(data);
	if(fc == NULL)
		goto err;

	if(fc->sb != NULL) {
		printk("fuse_read_super: file already mounted\n");
		goto err;
	}

        sb->u.generic_sbp = fc;
        sb->s_blocksize = 1024;
        sb->s_blocksize_bits = 10;
        sb->s_magic = FUSE_SUPER_MAGIC;
        sb->s_op = &fuse_super_operations;

	root = iget(sb, 1);
	if(!root) {
		printk("fuse_read_super: failed to get root inode\n");
		goto err;
	}

	printk(KERN_DEBUG "root inode: %ld/%d\n", root->i_ino, root->i_dev);

	root->i_mode = S_IFDIR;
	root->i_uid = 0;
	root->i_gid = 0;
	root->i_nlink = 2;
	root->i_size = 0;
	root->i_blksize = 1024;
	root->i_blocks = 0;
	root->i_atime = CURRENT_TIME;
	root->i_mtime = CURRENT_TIME;
	root->i_ctime = CURRENT_TIME;
	fuse_dir_init(root);

	sb->s_root = d_alloc_root(root);
	if(!sb->s_root) {
		printk("fuse_read_super: failed to allocate root dentry\n");
		goto err_iput;
	}

	fc->sb = sb;

	return sb;

  err_iput:
	iput(root);
  err:
	return NULL;
}


static DECLARE_FSTYPE(fuse_fs_type, "fuse", fuse_read_super, 0);

int fuse_fs_init()
{
	int res;

	res = register_filesystem(&fuse_fs_type);
	if(res)
		printk("fuse: failed to register filesystem\n");

	return res;
}

void fuse_fs_cleanup()
{
	unregister_filesystem(&fuse_fs_type);
}

/* 
 * Local Variables:
 * indent-tabs-mode: t
 * c-basic-offset: 8
 * End:
 */

