#include "userprog/syscall.h"
#include <stdio.h>
#include "devices/shutdown.h"
#include "process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/vaddr.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
static int get_user (const uint8_t *uaddr);
static bool put_user (uint8_t *udst, uint8_t byte);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

 	
/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}
 
/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

// From Qibo Wang's Pintos Project 2 starter video
static void
copy_in (void *dst_, const void*usrc_, size_t size)
{
  uint8_t *dst = dst_;
  const uint8_t *usrc = usrc_;

  for (; size > 0; size--, dst++, usrc++)
    *dst = get_user (usrc);
}


void 
halt (void)
{
  shutdown_power_off();
}
void 
exit (int status)
{
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit(status);
}

pid_t
exec (const char *cmd_line)
{
  return process_execute(cmd_line);
}

int
wait (pid_t pid)
{
  return process_wait(pid);
}

int
write (int fd, const void *buffer, unsigned size)
{
  if (fd == 1)
  {
    putbuf(buffer, size);
    return size;
  }
  else
    return -1;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  if (!is_user_vaddr (f->esp))
    exit (-1);
  unsigned syscall_number;
  int args[3];

  // Extracting syscall number
  copy_in (&syscall_number, f->esp, sizeof (syscall_number));

  switch (syscall_number)
  {
    case SYS_HALT:
      halt ();
      break;  
    case SYS_EXIT:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args));
      exit(args[0]);
      break;
    case SYS_EXEC:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args));
      f->eax = exec((const char*)args[0]);
      break;
    case SYS_WAIT:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args));
      f->eax = wait((pid_t)args[0]);
      break;
    case SYS_WRITE:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args) * 3);
      f->eax = write((int)args[0], (const void*)args[1], (unsigned)args[2]);
      break;
  }
}
