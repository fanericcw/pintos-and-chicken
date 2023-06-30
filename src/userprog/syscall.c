#include "userprog/syscall.h"
#include <stdio.h>
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
  // TODO: IMPLEMENT ERROR CHECKING
  uint8_t *dst = dst_;
  const uint8_t *usrc = usrc_;

  for (; size > 0; size--, dst++, usrc++)
    *dst = get_user (usrc);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // TODO: REWRITE THIS SECTION
  unsigned syscall_number;
  int args[3];

  // Extracting syscall number
  copy_in (&syscall_number, f->esp, sizeof (syscall_number));
  printf("Calling syscall: %u\n", syscall_number);

  if (syscall_number == 1) {
    thread_exit();
  }
  if (syscall_number == 9) {
    copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args) * 3);
    // printf("fd: %d (should be %u)\n", args[0], STDOUT_FILENO);
    // printf("buffer address: %p\n", args[1]);
    // printf("size: %d\n", args[2]);

    // write on STDOUT_FILENO
    putbuf (args[1], args[2]);

    // set the returned value
    f->eax = args[2];
  }
}
