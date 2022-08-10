#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include "fifoqueue.h"

MODULE_LICENSE("GPL");

void add_item(struct FifoQueue *p, const char *str)
{
  unsigned int len = 0;
  char *new_p;
  if (((p->head + 1) % p->len) == p->tail)
  {
    return;
  }
  len = strlen(str) > 254 ? 254 : strlen(str);
  new_p = kmalloc(len + 1, GFP_KERNEL);
  memcpy(new_p, str, len);
  new_p[len] = 0;
  p->array[p->head] = new_p;
  p->head = (p->head + 1) % p->len;
}

void remove_item(struct FifoQueue *p)
{
  if (p->tail != p->head)
  {
    kfree(p->array[p->tail]);
    p->array[p->tail] = 0;
    p->tail = (p->tail + 1) % p->len;
  }
}

char *get_item_from_top(struct FifoQueue *p)
{
  if (p->tail == p->head)
  {
    return "";
  }
  return p->array[p->tail];
}

void cleanup_queue(struct FifoQueue *p)
{
  while (p->tail != p->head)
  {
    kfree(p->array[p->tail]);
    p->array[p->tail] = 0;
    p->tail = (p->tail + 1) % p->len;
  }
}

unsigned int list_of_items_in_queue(struct FifoQueue *p)
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
