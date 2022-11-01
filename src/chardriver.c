#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include "chardriver.h"
#include "ringbuffer.h"

MODULE_LICENSE("GPL");

static DEFINE_MUTEX(mtx_read);
static DEFINE_MUTEX(mtx_write);

DECLARE_WAIT_QUEUE_HEAD(read_queue);

static int major_number;
static int number_opens = 0;
static struct class *startup_monitor_class = NULL;
static struct device *startup_monitor_device = NULL;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

struct ring_buffer buf_ring = {
    .head = 0,
    .tail = 0,
    .len = NUM_QUEUE,
};

static int dev_open(struct inode *inodep, struct file *filep)
{
  number_opens++;
  // printk(KERN_INFO "%s: Device has been opened %d times\n", CLASS_NAME, number_opens);
  return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
  int error_count = 0;
  int bytes_read;
  char *str;
  int res;

  res = wait_event_interruptible(read_queue, strlen(get_item_from_top(&buf_ring)) != 0);
  if (res)
  {
    printk(KERN_INFO "%s: Signal Exit\n", CLASS_NAME);
    return res;
  }

  mutex_lock(&mtx_read);
  str = get_item_from_top(&buf_ring);

  bytes_read = strlen(str);
  error_count = copy_to_user(buffer, str, bytes_read);

  remove_item(&buf_ring);

  mutex_unlock(&mtx_read);

  if (error_count)
  {
    printk(KERN_INFO "%s: Failed to send %d characters to the user\n", CLASS_NAME, error_count);
    return -EFAULT;
  }

  return bytes_read;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
  char str[256];
  unsigned int size = len > 254 ? 254 : len;

  if (copy_from_user(str, buffer, size))
  {
    printk(KERN_ALERT "%s: Unable to read buffer from user\n", CLASS_NAME);
    return -EFAULT;
  }

  str[size] = 0;

  add_item_to_queue(str);

  return len;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
  // printk(KERN_INFO "%s: Device successfully closed\n", CLASS_NAME);
  number_opens--;
  return 0;
}

void add_item_to_queue(const char *str)
{
  mutex_lock(&mtx_write);
  add_item(&buf_ring, str);
  mutex_unlock(&mtx_write);

  wake_up_interruptible(&read_queue);
}

int create_char_device(void)
{
  major_number = register_chrdev(0, DEVICE_NAME, &fops);
  if (major_number < 0)
  {
    printk(KERN_ALERT "%s: Failed to register a major number\n", CLASS_NAME);
    return -1;
  }

  printk(KERN_INFO "%s: Registered correctly with major number %d\n", CLASS_NAME, major_number);

  startup_monitor_class = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(startup_monitor_class))
  {
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_ALERT "%s: Failed to create the device\n", CLASS_NAME);
    return -1;
  }

  printk(KERN_INFO "%s: Device class registered correctly\n", CLASS_NAME);

  startup_monitor_device = device_create(startup_monitor_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
  if (IS_ERR(startup_monitor_device))
  {
    class_destroy(startup_monitor_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_ALERT "%s: Failed to create the device\n", CLASS_NAME);
    return -1;
  }

  printk(KERN_INFO "%s: Character device created correctly\n", CLASS_NAME);
  return 0;
}

void remove_char_device(void)
{
  device_destroy(startup_monitor_class, MKDEV(major_number, 0));
  // class_unregister(ncrpsmonClass);
  class_destroy(startup_monitor_class);
  unregister_chrdev(major_number, DEVICE_NAME);

  mutex_lock(&mtx_read);
  cleanup_queue(&buf_ring);
  mutex_unlock(&mtx_read);

  printk(KERN_INFO "%s: Character device removed\n", CLASS_NAME);
}