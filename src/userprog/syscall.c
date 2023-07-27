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
#include "pagedir.h"

static void syscall_handler (struct intr_frame *);
static int get_user (const uint8_t *uaddr);
static bool put_user (uint8_t *udst, uint8_t byte);

 /* An autoincremented unique number for each fd */
static int fd_num = 1;           
/* List of all open files */
struct list all_files;
/* Locks for synch */
struct lock file_lock;
struct lock load_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  list_init(&all_files);
  lock_init(&file_lock);
  lock_init(&load_lock);
}

 	
/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  if (!is_user_vaddr(uaddr) || !pagedir_get_page(
                      thread_current()->pagedir, uaddr))
    return -1;
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
  if (list_empty(&all_files))
    return NULL;

  struct sys_file *temp;
  struct list_elem *e = list_front(&all_files);
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

/* Find child_process by TID*/
struct child_process *
find_child(tid_t tid, struct list child_list)
{
  struct list_elem *e;
  struct child_process *c;

  for (e = list_begin (&child_list); e != list_end (&child_list);
       e = list_next (e))
  {
    c = list_entry (e, struct child_process, child_elem);
    if (c->tid == tid)
      return c;
  }
  return NULL;
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
  /* Check end of cmd_line instead of start to see if it execceds pg bound */
  if (!is_user_vaddr(cmd_line) || 
  !pagedir_get_page(thread_current()->pagedir, cmd_line + sizeof(cmd_line)))
    exit(-1);
  struct thread *parent = thread_current();
  tid_t pid = -1;
  lock_acquire(&load_lock);
  pid = process_execute(cmd_line);
  lock_release(&load_lock);

  struct child_process *child = find_child(pid, parent->child_list);
  /* Wait for child to finish */
  sema_down(&child->waiting);
  if (child->exit_status == -1) {
    return -1;
  }
  return pid;
}

int
wait (pid_t pid)
{
  return process_wait(pid);
}

bool
create(const char *file, unsigned initial_size)
{
  if (!is_user_vaddr(file) || !pagedir_get_page(
                      thread_current()->pagedir, file))
    exit(-1);
  lock_acquire(&file_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&file_lock);
  return success;
}

bool 
remove(const char *file)
{ 
  if (!is_user_vaddr(file) || !pagedir_get_page(
                      thread_current()->pagedir, file))
    exit(-1);
  lock_acquire(&file_lock);
  bool success = filesys_remove(file);
  lock_release(&file_lock);
  return success;
}

int open(const char *file)
{
  if (!is_user_vaddr(file) || !pagedir_get_page(
                      thread_current()->pagedir, file))
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
  struct sys_file *sysfile;
  if (!is_user_vaddr(buffer) || !pagedir_get_page(
                      thread_current()->pagedir, buffer))
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
  sysfile = get_sys_file(fd);
  if (sysfile == NULL) {
    return -1;
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
  struct sys_file *sysfile;
  if (!is_user_vaddr(buffer) || !pagedir_get_page(
                          thread_current()->pagedir, buffer))
    exit(-1);
  if (fd == STDIN_FILENO)
    return -1;
  if (fd == STDOUT_FILENO)
  {
    putbuf(buffer, size);
    return size;
  }
  sysfile = get_sys_file(fd);
  if (sysfile == NULL) {
    return -1;
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
  struct sys_file *sys_file = get_sys_file(fd);
  if (sys_file == NULL)
    exit(-1);
  if (sys_file->fd_owner != thread_current()->tid)
    exit(-1);
  file_close(sys_file->file);
  list_remove(&sys_file->file_elem);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  if (!is_user_vaddr (f->esp) || !pagedir_get_page (
                      thread_current ()->pagedir, f->esp))
    exit (-1);
  unsigned syscall_number;
  int args[3];

  thread_current()->esp = f->esp;

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
    case SYS_SEEK:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args) * 2);
      seek((int)args[0], (unsigned)args[1]);
      break;
    case SYS_TELL:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args));
      f->eax = tell((int)args[0]);
      break;
    case SYS_CLOSE:
      copy_in (args, (uint32_t *) f->esp + 1, sizeof (*args));
      close((int)args[0]);
      break;
    default:
      exit(-1);
      break;
  }
}
