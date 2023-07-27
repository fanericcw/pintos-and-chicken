#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

typedef int mapid_t;

// #ifdef VM
struct mmap_details {
  mapid_t id;
  struct list_elem elem;
  struct file* file;        // File

  void *addr;               // User virtual address
  size_t size;              // File size
};
// #endif


tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (int status);
void process_activate (void);

#endif /* userprog/process.h */
