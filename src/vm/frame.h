#ifndef FRAME_H
#define FRAME_H

#include "threads/thread.h"
#include "threads/palloc.h"

struct frame 
{
    struct thread *thread;
    void *user_virt_addr;
    void *kernel_virt_addr;
    struct list_elem frame_elem;
};  

struct list frame_table;
struct list_elem *clk_hand_ptr;

void frame_init (void);
void *allocate_frame (enum palloc_flags flags, void *user_virt_addr);
void add_frame (void* kernel_virt_addr, void *user_virt_addr);
void free_frame (void *user_virt_addr);
void *evict_frame (void);

#endif /* vm/frame.h */