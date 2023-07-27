#include <stdio.h> 
#include <stdlib.h>
#include "lib/kernel/list.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/swap.h"

void 
frame_init (void)
{
    list_init (&frame_table);
    clk_hand_ptr = NULL;
}

void *
allocate_frame (enum palloc_flags flags, void *user_virt_addr)
{
    void *kernel_virt_addr = palloc_get_page (flags);
    if (kernel_virt_addr == NULL)
        return NULL;       

    if (kernel_virt_addr != NULL)
        add_frame (kernel_virt_addr, user_virt_addr);
    else
    {
        kernel_virt_addr = evict_frame ();
        add_frame (kernel_virt_addr, user_virt_addr);
    }

    return kernel_virt_addr;
}

void
add_frame (void *kernel_virt_addr, void *user_virt_addr)
{
    struct frame *f = malloc(sizeof(struct frame));
    f->thread = thread_current ();
    f->kernel_virt_addr = kernel_virt_addr;
    f->user_virt_addr = user_virt_addr;
    list_push_back (&frame_table, &(f->frame_elem));
}

void 
free_frame (void *kernel_virt_addr)
{
    struct frame *f;
    struct list_elem *e;
    for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e))
    {
        f = list_entry (e, struct frame, frame_elem);
        if (f->kernel_virt_addr == kernel_virt_addr)
        {
            list_remove (e);
            palloc_free_page (f->kernel_virt_addr);
            free (f);
            break;
        }
    }
}

void *
evict_frame (void)
{
    struct frame *f;

    if (clk_hand_ptr == NULL || clk_hand_ptr == list_end (&frame_table))
        clk_hand_ptr = list_begin (&frame_table);
    else
        clk_hand_ptr = list_next (clk_hand_ptr);

    while (true)
    {
        if (clk_hand_ptr == list_end (&frame_table))
            clk_hand_ptr = list_begin (&frame_table);
        f = list_entry (clk_hand_ptr, struct frame, frame_elem);
        if (pagedir_is_accessed (f->thread->pagedir, f->user_virt_addr))
            pagedir_set_accessed (f->thread->pagedir, f->user_virt_addr, false);
        else
        {
            if (pagedir_is_dirty (f->thread->pagedir, f->user_virt_addr))
            {
                uint32_t swap_index = swap_out (f->user_virt_addr);
                pagedir_clear_page (f->thread->pagedir, f->user_virt_addr);
                pagedir_set_dirty (f->thread->pagedir, f->user_virt_addr, false);
                return swap_index;
            }
            else
            {
                pagedir_clear_page (f->thread->pagedir, f->user_virt_addr);
                return f->user_virt_addr;
            }
        }
        clk_hand_ptr = list_next (&frame_table);
    }
}