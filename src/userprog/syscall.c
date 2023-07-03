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
#include "lib/kernel/list.h"

static void syscall_handler (struct intr_frame *);
static int get_user (const uint8_t *uaddr);
static bool put_user (uint8_t *udst, uint8_t byte);

 /* An autoincremented unique number for each fd */
static int fd_num = 1;           
/* List of all open files */
struct list all_files;
/* Lock for synch */
struct lock call_lock;        

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init(&all_files);
  lock_init(&call_lock);
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

struct sys_file*
get_sys_file(int fd)
{
  struct sys_file *temp;
  struct list_elem *e = list_front(&all_files);
  if (list_empty(&all_files))
    temp = NULL;
  /* Find corresponding file_elem in list*/
  while (1)
  {
    temp = list_entry(e, struct sys_file, file_elem);
    if (temp->fd == fd)
    {
      return temp;
    }
    if (e == list_tail(&all_files))
      break;
    e = list_next(e);
  }
}

// bool
// check_bad_ptr (void * ptr) 
// {
//   if (ptr > PHYS_BASE || ptr < return_address) {
    
//   }
// }

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
  if (!is_user_vaddr(cmd_line) || !pagedir_get_page(thread_current()->pagedir, cmd_line))
    exit(-1);
  return process_execute(cmd_line);
}

/* Above requires error checking */
/* Below needs synch */

int
wait (pid_t pid)
{
  return process_wait(pid);
}

/* TODO: Synchronize calls */
bool
create(const char *file, unsigned initial_size)
{
  if (!is_user_vaddr(file) || !pagedir_get_page(thread_current()->pagedir, file))
    exit(-1);
  lock_acquire(&call_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&call_lock);
  return success;
}

// TODO: Implement removing open file
bool 
remove(const char *file)
{ 
  if (!is_user_vaddr(file) || !pagedir_get_page(thread_current()->pagedir, file))
    exit(-1);
  lock_acquire(&call_lock);
  bool success = filesys_remove(file);
  lock_release(&call_lock);
  return success;
}

int open(const char *file)
{
  if (!is_user_vaddr(file) || !pagedir_get_page(thread_current()->pagedir, file))
    exit(-1);
  struct file *open_file;
  struct sys_file *sys_file = malloc(sizeof(*sys_file));

  if (file == NULL) 
    return -1;

  open_file = filesys_open(file);
  if (open_file == NULL) 
    return -1;
  sys_file->fd = ++fd_num;
  sys_file->file = open_file;
  sys_file->fd_owner = thread_current ()->tid;
  list_push_front(&all_files, &(sys_file->file_elem));
  return sys_file->fd;
}

int filesize(int fd)
{
  struct sys_file *sys_file = get_sys_file(fd);
  if (sys_file == NULL)
    return 0;
  return file_length(sys_file->file);
}

int 
read (int fd, void *buffer, unsigned size)
{
  if (!is_user_vaddr(buffer) || !pagedir_get_page(thread_current()->pagedir, buffer))
    exit(-1);
  if (fd == STDOUT_FILENO)
    return -1;
  if (fd == STDIN_FILENO)
  {
    int actual_read = 0;
    for (int i = 0; i < size; i++) {
      uint8_t in = input_getc();
      if (in != NULL) {
        buffer = (char) in;  // Convert byte to extended ASCII?
        buffer++;
        actual_read++;
      }
    }
    return actual_read;
  }
  else
  {
    struct sys_file *sys_file = get_sys_file(fd);
    if (sys_file == NULL)
      return 0;
    return file_read(sys_file->file, buffer, size);
  }
}

int
write (int fd, const void *buffer, unsigned size)
{
  if (!is_user_vaddr(buffer) || !pagedir_get_page(thread_current()->pagedir, buffer))
    exit(-1);
  if (fd == STDIN_FILENO)
    return -1;
  if (fd == STDOUT_FILENO)
  {
    putbuf(buffer, size);
    return size;
  }
  else
  {
    struct sys_file *sys_file = get_sys_file(fd);
    if (sys_file == NULL)
      return 0;
    return file_write(sys_file->file, buffer, size);
  }
}
void 
seek (int fd, unsigned position)
{
  struct sys_file *sys_file = get_sys_file(fd);
  file_seek(sys_file->file, position);
}
unsigned 
tell (int fd)
{
  struct sys_file *sys_file = get_sys_file(fd);
  file_tell(sys_file->file);
}
void 
close (int fd)
{
  return;
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
    case SYS_CREATE:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args) * 2);
      f->eax = create((const char*) args[0], (unsigned) args[1]);
      break;
    case SYS_REMOVE:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args));
      f->eax = remove((const char*) args[0]);
      break;
    case SYS_OPEN:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args));
      f->eax = open((const char*)args[0]);
      break;
    case SYS_FILESIZE:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args));
      f->eax = filesize((int)args[0]);
      break;
    case SYS_READ:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args) *3);
      f->eax = read((int)args[0], (void*)args[1], (unsigned)args[2]);
      break;
    case SYS_WRITE:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args) * 3);
      f->eax = write((int)args[0], (const void*)args[1], (unsigned)args[2]);
      break;
  }
}
