#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include "chardriver.h"
#include "fifoqueue.h"

MODULE_LICENSE("GPL");

static DEFINE_MUTEX(mtx_Read);
static DEFINE_MUTEX(mtx_Write);

DECLARE_WAIT_QUEUE_HEAD(readQueue);

static int majorNumber;
static int numberOpens = 0;
static struct class *startupMonitorClass = NULL;
static struct device *startupMonitorDevice = NULL;

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

struct FifoQueue fque = {
    .head = 0,
    .tail = 0,
    .len = NUM_QUEUE,
};

static int dev_open(struct inode *inodep, struct file *filep)
{
  numberOpens++;
  // printk(KERN_INFO "%s: Device has been opened %d times\n", CLASS_NAME, numberOpens);
  return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
  int error_count = 0;
  int bytes_read;
  char *str;
  int res;

  res = wait_event_interruptible(readQueue, strlen(get_item_from_top(&fque)) != 0);
  if (res)
  {
    printk(KERN_INFO "%s: Signal Exit\n", CLASS_NAME);
    return res;
  }

  mutex_lock(&mtx_Read);
  str = get_item_from_top(&fque);

  bytes_read = strlen(str);
  error_count = copy_to_user(buffer, str, bytes_read);

  remove_item(&fque);

  mutex_unlock(&mtx_Read);

  if (error_count)
  {
    printk(KERN_INFO "%s: Failed to send %d characters to the user\n", CLASS_NAME, error_count);
    return -EFAULT;
  }

  // printk(KERN_INFO "%s: Sent %d characters to the user\n", CLASS_NAME, bytes_read);

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

  // printk(KERN_INFO "%s: Received %zu characters from the user\n", CLASS_NAME, len);
  //  printk(KERN_INFO "%s: Write: %s\n", CLASS_NAME, message);

  return len;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
  // printk(KERN_INFO "%s: Device successfully closed\n", CLASS_NAME);
  numberOpens--;
  return 0;
}

void add_item_to_queue(const char *str)
{
  mutex_lock(&mtx_Write);
  add_item(&fque, str);
  mutex_unlock(&mtx_Write);

  wake_up_interruptible(&readQueue);
}

int create_char_device(void)
{
  majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
  if (majorNumber < 0)
  {
    printk(KERN_ALERT "%s: Failed to register a major number\n", CLASS_NAME);
    return -1;
  }

  printk(KERN_INFO "%s: Registered correctly with major number %d\n", CLASS_NAME, majorNumber);

  startupMonitorClass = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(startupMonitorClass))
  {
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_ALERT "%s: Failed to create the device\n", CLASS_NAME);
    return -1;
  }

  printk(KERN_INFO "%s: Device class registered correctly\n", CLASS_NAME);

  startupMonitorDevice = device_create(startupMonitorClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
  if (IS_ERR(startupMonitorDevice))
  {
    class_destroy(startupMonitorClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_ALERT "%s: Failed to create the device\n", CLASS_NAME);
    return -1;
  }

  printk(KERN_INFO "%s: Character device created correctly\n", CLASS_NAME);
  return 0;
}

void remove_char_device(void)
{
  device_destroy(startupMonitorClass, MKDEV(majorNumber, 0));
  // class_unregister(ncrpsmonClass);
  class_destroy(startupMonitorClass);
  unregister_chrdev(majorNumber, DEVICE_NAME);

  mutex_lock(&mtx_Read);
  cleanup_queue(&fque);
  mutex_unlock(&mtx_Read);

  printk(KERN_INFO "%s: Character device removed\n", CLASS_NAME);
}