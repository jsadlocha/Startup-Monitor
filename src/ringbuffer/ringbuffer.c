#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>

#include "ringbuffer.h"
#include "../chardriver/chardriver.h"

MODULE_LICENSE("GPL");

void add_item(struct ring_buffer *p, const char *str)
{
  unsigned int len = 0;
  char *new_p;
  if (((p->head + 1) % p->len) == p->tail)
  {
    return;
  }
  len = strlen(str) > 254 ? 254 : strlen(str);
  new_p = kmalloc(len + 1, GFP_KERNEL);

  if (new_p == 0)
  {
    printk(KERN_ALERT "%s: Memory allocation error!\n", CLASS_NAME);
    return;
  }
  memcpy(new_p, str, len);
  new_p[len] = 0;
  p->array[p->head] = new_p;
  p->head = (p->head + 1) % p->len;
}

static void __remove(struct ring_buffer *p)
{
  kfree(p->array[p->tail]);
  p->array[p->tail] = 0;
  p->tail = (p->tail + 1) % p->len;
}

void remove_item(struct ring_buffer *p)
{
  if (p->tail != p->head)
    __remove(p);
}

char *get_item_from_top(struct ring_buffer *p)
{
  if (p->tail == p->head)
  {
    return "";
  }
  return p->array[p->tail];
}

void cleanup_queue(struct ring_buffer *p)
{
  while (p->tail != p->head)
    __remove(p);
}

unsigned int list_of_items_in_queue(struct ring_buffer *p)
{
  unsigned int size = 0;
  unsigned int tail = p->tail;
  while (tail != p->head)
  {
    tail = (tail + 1) % p->len;
    size += 1;
  }
  return size;
}
