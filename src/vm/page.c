#include <stdio.h>
#include <stdlib.h>
#include "vm/page.h"
#include "threads/malloc.h"
#include "lib/kernel/list.h"
#include "threads/thread.h"
#include "vm/frame.h"
#include "userprog/pagedir.h"

// Destroys entire SPT for a thread
void spt_destroy (struct list *spt) {
  ASSERT(spt != NULL)
  struct thread *cur = thread_current();

  struct list_elem *e;
  for (e = list_begin(spt); e != list_end(spt);
  e = e) 
  {
    struct spte *entry = list_entry(e, struct spte, elem);
    free_frame(pagedir_get_page(cur->pagedir, entry->user_virt_addr));
		pagedir_clear_page(cur->pagedir, entry->user_virt_addr);
  }
}

bool
spte_set_page (struct list *spt, void *upage)
{
  struct spte *spte;
  spte = (struct spte *) malloc(sizeof(struct spte));
  // struct thread *cur = thread_current();
  bool in_list = false;  // upage already inserted?

  spte->user_virt_addr = upage;
  spte->state = FRAME;

  struct list_elem *e;
  for (e = list_begin(spt); e != list_end(spt);
  e = list_next(e)) 
  {
      struct spte *entry = list_entry(e, struct spte, elem);
      if (entry->user_virt_addr == upage) {
          in_list = true;
      }
  }

  if (!in_list) {
    list_push_back(spt, &spte->elem);
    // successfully inserted into the supplemental page table.
    return true;
  }
  else {
    // failed. there is already an entry.
    free (spte);
    return false;
  }
}