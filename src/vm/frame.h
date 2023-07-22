#ifndef FRAME_H
#define FRAME_H

#include "threads/thread.h"
#include "threads/palloc.h"

struct frame 
{
    struct thread *thread;
    void *user_virt_addr;
    struct list_elem frame_elem;
};  

struct list frame_table;

void frame_init (void);
void *allocate_frame (enum palloc_flags flags);
void add_frame (void *user_virt_addr);
void free_frame (void *user_virt_addr);

#endif /* vm/frame.h */