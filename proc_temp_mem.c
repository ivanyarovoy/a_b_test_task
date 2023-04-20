#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

#define PROCFS_NAME "test_task"
#define MAX_SINGLE_ENTRY 512

static struct proc_dir_entry *our_proc_file;

static char *procfs_buffer = NULL;

static unsigned long procfs_buffer_size = 0;

static ssize_t procfile_read(struct file *filePointer, char __user *buffer,
                             size_t buffer_length, loff_t *offset) {
  int len = strlen(procfs_buffer);
  ssize_t ret = len;
  if (*offset >= len || copy_to_user(buffer, procfs_buffer, len)) {
    pr_info("copy_to_user failed\n");
    ret = 0;
  } else {
    pr_info("procfile read %s\n", filePointer->f_path.dentry->d_name.name);
    *offset += len;
  }
  return ret;
}

static ssize_t procfile_write(struct file *file, const char __user *buff,
                              size_t len, loff_t *off) {
  char data_to_write[MAX_SINGLE_ENTRY];
  unsigned int entry_len = len;
  if (entry_len > MAX_SINGLE_ENTRY)
    entry_len = MAX_SINGLE_ENTRY;

  if (copy_from_user(data_to_write, buff, entry_len))
    return -EFAULT;

  data_to_write[(entry_len < MAX_SINGLE_ENTRY) ? entry_len
                                               : MAX_SINGLE_ENTRY - 1] = '\0';
  procfs_buffer_size = strlen(procfs_buffer) + strlen(data_to_write);
  procfs_buffer =
      (char *)krealloc(procfs_buffer, procfs_buffer_size, GFP_KERNEL);
  if (NULL == procfs_buffer) {
    pr_alert("Error: could not allocate memory");
    return -ENOMEM;
  }
  strcat(procfs_buffer, data_to_write);
  pr_info("procfile write %s\n", procfs_buffer);
  return procfs_buffer_size;
}

#ifdef HAVE_PROC_OPS
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
    .write = procfile_write,
};
#endif

static int __init proc_temp_mem_init(void) {
  our_proc_file = proc_create(PROCFS_NAME, 0666, NULL, &proc_file_fops);
  if (NULL == our_proc_file) {
    proc_remove(our_proc_file);
    pr_alert("Error:Could not initialize /proc/%s\n", PROCFS_NAME);
    return -ENOMEM;
  }
  procfs_buffer = (char *)kmalloc(sizeof(char), GFP_KERNEL);
  if (NULL == procfs_buffer) {
    proc_remove(our_proc_file);
    pr_alert("Error:could not allocate memory");
    return -ENOMEM;
  }
  procfs_buffer[0] = '\0';
  pr_info("/proc/%s created\n", PROCFS_NAME);
  return 0;
}

static void __exit proc_temp_mem_exit(void) {
  proc_remove(our_proc_file);
  kfree(procfs_buffer);
  pr_info("/proc/%s removed\n", PROCFS_NAME);
}

module_init(proc_temp_mem_init);
module_exit(proc_temp_mem_exit);

MODULE_LICENSE("GPL");
