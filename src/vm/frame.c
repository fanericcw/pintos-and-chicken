#include <stdio.h> 
#include <stdlib.h>
#include "lib/kernel/list.h"
#include "threads/malloc.h"
#include "vm/frame.h"

void 
frame_init (void)
{
    list_init (&frame_table);
}

void *
allocate_frame (enum palloc_flags flags)
{
    void *user_virt_addr = palloc_get_page (flags);
    if (user_virt_addr == NULL)
        return NULL;       
    add_frame (user_virt_addr);
    return user_virt_addr;
}

void
add_frame (void *user_virt_addr)
{
    struct frame *f = malloc(sizeof(struct frame));
    f->thread = thread_current ();
    f->user_virt_addr = user_virt_addr;
    list_push_back (&frame_table, &(f->frame_elem));
}

void 
free_frame (void *user_virt_addr)
{
    struct frame *f;
    struct list_elem *e;
    for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e))
    {
        f = list_entry (e, struct frame, frame_elem);
        if (f->user_virt_addr == user_virt_addr)
        {
            list_remove (e);
            palloc_free_page (f->user_virt_addr);
            free (f);
            break;
        }
    }
}

