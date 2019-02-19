/* list.h:
   doubly linked list 
 */

#ifndef __STRUCT_LIST__
#define __STRUCT_LIST__

#ifndef NULL
#define NULL 0
#endif

typedef struct list_head {
    struct list_head *prev, *next;
}list_head;

// initialize list
#define INIT_LIST_HEAD(ptr) do {\
    struct list_head *_ptr = (struct list_head *)ptr;   \
    (_ptr)->next = (_ptr);  \
    (_ptr->prev) = (_ptr);  \
}while(0)

// get the address shift between MEMBER and TYPE first address
#define loffsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

// get the first address of struct type containing member whose address is ptr
#define container_of(ptr, type, member) ({  \
    const typeof(((type *)0)->member) *_mptr = (ptr);   \
    (type *)((char *)_mptr - loffsetof(type,member));    \
})

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_prev(pos, head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)

// insert new node
static inline void __list_add(struct list_head *_new, struct list_head *prev, struct list_head *next) {
    _new->next = next;
    next->prev = _new;
    prev->next = _new;
    _new->prev = prev;
}

// insert new node to head
static inline void list_add(struct list_head *_new, struct list_head *head) {
    __list_add(_new, head, head->next);
}

// insert new node to tail
static inline void list_add_tail(struct list_head *_new, struct list_head *head) {
    __list_add(_new, head->prev, head);
}

// delete node
static inline void __list_del(struct list_head *prev, struct list_head *next) {
    prev->next = next;
    next->prev = prev;
}

// delete entry node
static inline void list_del(struct list_head *entry) {
    __list_del(entry->prev, entry->next);
}

// judge whether list is empty
static inline int list_empty(struct list_head *head) {
    return (head->next == head) && (head->prev == head);
}

#endif
