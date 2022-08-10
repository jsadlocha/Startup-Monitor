#include <linux/kprobes.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>

#include "chardriver.h"

MODULE_LICENSE("GPL");

bool get_user_cmdline(const char __user *const __user *argv, char *cmdline, int cmd_len);

typedef asmlinkage long (*execve_t)(const struct pt_regs *);

typedef asmlinkage long (*sys_call_ptr_t)(const struct pt_regs *);
static sys_call_ptr_t *sys_call_table;

execve_t orig_execve = NULL;

asmlinkage long hook_execve(const struct pt_regs *regs);

static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name"};

inline void cr0_write(unsigned long cr0)
{
  asm volatile("mov %0,%%cr0"
               :
               : "r"(cr0)
               : "memory");
}

static inline void protect_memory(void)
{
  unsigned long cr0 = read_cr0();
  set_bit(16, &cr0);
  cr0_write(cr0);
}

static inline void unprotect_memory(void)
{
  unsigned long cr0 = read_cr0();
  clear_bit(16, &cr0);
  cr0_write(cr0);
}

asmlinkage long hook_execve(const struct pt_regs *regs)
{
  char cmdline[255];
  char buffer[255];

  if (get_user_cmdline((const char __user *const __user *)regs->si, cmdline, 255))
  {
    snprintf(buffer, 255, "Execve: %s\n", cmdline);
    add_item_to_queue(buffer);
  }

  return orig_execve(regs);
}

void syscall_hook_init(void)
{
  typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);
  kallsyms_lookup_name_t kallsyms_lookup_name;

  register_kprobe(&kp);
  kallsyms_lookup_name = (kallsyms_lookup_name_t)kp.addr;
  unregister_kprobe(&kp);

  sys_call_table = (sys_call_ptr_t *)kallsyms_lookup_name("sys_call_table");

  orig_execve = (execve_t)sys_call_table[__NR_execve];

  unprotect_memory();
  sys_call_table[__NR_execve] = (sys_call_ptr_t)hook_execve;
  protect_memory();
}

void syscall_hook_remove(void)
{
  unprotect_memory();
  sys_call_table[__NR_execve] = (sys_call_ptr_t)orig_execve;
  protect_memory();
}

bool get_user_cmdline(const char __user *const __user *argv, char *cmdline, int cmd_len)
{
  int i = 0;
  int offset = 0;
  char tmp[256] = {0};
  if (unlikely(argv == NULL || cmdline == NULL || cmd_len <= 0))
    return false;
  memset(cmdline, 0, cmd_len);

  if (argv != NULL)
  {
    for (; i < 0x7fffffff;)
    {
      const char __user *p;
      int ret = get_user(p, argv + i);
      if (ret || !p || IS_ERR(p))
      {
        break;
      }

      ret = copy_from_user(tmp, p, 256);
      if (ret < 256)
      {
        int tmp_len = strlen(tmp);
        if (offset + 1 + tmp_len > cmd_len)
        {
          break;
        }
        strncpy(cmdline + offset, tmp, tmp_len);
        offset += tmp_len;
        cmdline[offset] = ' ';
        offset++;
      }
      ++i;
    }
  }
  if (cmdline[offset - 1] == ' ')
    cmdline[offset - 1] = 0;
  return true;
}