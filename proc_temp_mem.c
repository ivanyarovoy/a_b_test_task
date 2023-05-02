#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
#define HAVE_PROC_OPS
#endif

#define PROCFS_NAME "test_task"

struct node {
  char *data;
  size_t length;
  struct node *next;
};

struct node *head = NULL;

size_t overall_length = 0;

struct mutex rw_mutex;

static struct proc_dir_entry *our_proc_file;

static ssize_t procfile_read(struct file *filePointer, char __user *buffer,
                             size_t buffer_length, loff_t *offset) {
  mutex_lock(&rw_mutex);
  if (*offset >= overall_length) {
    mutex_unlock(&rw_mutex);
    return 0;
  }
  struct node *curr_element = head;
  size_t copied_symbols = 0;
  size_t skipped_symbols = 0;
  while (skipped_symbols + curr_element->length < *offset) {
    skipped_symbols += curr_element->length;
    curr_element = curr_element->next;
  }
  size_t position = *offset - skipped_symbols;
  while (copied_symbols < buffer_length) {
    if (*offset >= overall_length)
      break;
    if (buffer_length - copied_symbols >= curr_element->length - position) {
      if (copy_to_user(buffer + copied_symbols, curr_element->data + position,
                       curr_element->length - position)) {
        pr_alert("Error: copy_to_user failed\n");
        mutex_unlock(&rw_mutex);
        return -EFAULT;
      }
      *offset += curr_element->length - position;
      copied_symbols += curr_element->length - position;
      position = 0;
      if (curr_element->next == NULL)
        break;
      curr_element = curr_element->next;
    } else {
      if (copy_to_user(buffer + copied_symbols, curr_element->data + position,
                       buffer_length - copied_symbols)) {
        pr_alert("Error: copy_to_user failed\n");
        mutex_unlock(&rw_mutex);
        return -EFAULT;
      }
      *offset += (buffer_length - copied_symbols);
      copied_symbols += (buffer_length - copied_symbols);
    }
  }
  mutex_unlock(&rw_mutex);
  return copied_symbols;
}

static ssize_t procfile_write(struct file *file, const char __user *buff,
                              size_t len, loff_t *off) {
  struct node *new_element =
      (struct node *)kmalloc(sizeof(struct node), GFP_KERNEL);
  if (NULL == new_element) {
    pr_alert("Error: could not allocate memory\n");
    return -ENOMEM;
  }
  new_element->data = (char *)kmalloc(len * sizeof(char), GFP_KERNEL);
  if (NULL == new_element->data) {
    pr_alert("Error: could not allocate memory\n");
    return -ENOMEM;
  }
  new_element->length = len;
  new_element->next = NULL;
  if (copy_from_user(new_element->data, buff, len))
    return -EFAULT;
  mutex_lock(&rw_mutex);
  overall_length += len;
  if (head == NULL)
    head = new_element;
  else {
    struct node *element = head;
    while (element->next != NULL)
      element = element->next;
    element->next = new_element;
  }
  mutex_unlock(&rw_mutex);
  return new_element->length;
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
  mutex_init(&rw_mutex);
  our_proc_file = proc_create(PROCFS_NAME, 0666, NULL, &proc_file_fops);
  if (NULL == our_proc_file) {
    proc_remove(our_proc_file);
    pr_alert("Error:Could not initialize /proc/%s\n", PROCFS_NAME);
    return -ENOMEM;
  }
  pr_info("/proc/%s created\n", PROCFS_NAME);
  return 0;
}

static void __exit proc_temp_mem_exit(void) {
  proc_remove(our_proc_file);
  pr_info("/proc/%s removed\n", PROCFS_NAME);
  if (head == NULL)
    return;
  struct node *curr_element = head;
  struct node *prev_element = head;
  while (curr_element->next != NULL) {
    curr_element = curr_element->next;
    kfree(prev_element->data);
    kfree(prev_element);
    prev_element = curr_element;
  }
  kfree(curr_element->data);
  kfree(curr_element);
}

module_init(proc_temp_mem_init);
module_exit(proc_temp_mem_exit);

MODULE_LICENSE("GPL");
