#include <linux/module.h>
#include <linux/device.h>

#include "chardriver.h"
#include "ringbuffer.h"
#include "syscallhook.h"

#define AUTHOR "Jaroslaw Sadlocha"
#define DESC "Startup Applications Monitor"

static int __init startupMonitor_init(void)
{
    if (create_char_device() < 0)
        return -1;

    syscall_hook_init();

    printk(KERN_INFO "%s: Module initialized\n", CLASS_NAME);
    return 0;
}

static void __exit startupMonitor_remove(void)
{
    syscall_hook_remove();
    remove_char_device();
    printk(KERN_INFO "%s: Module Removed\n", CLASS_NAME);
}

module_init(startupMonitor_init);
module_exit(startupMonitor_remove);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(AUTHOR);
MODULE_DESCRIPTION(DESC);