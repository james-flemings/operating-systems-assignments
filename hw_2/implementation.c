/*  

    Copyright 2018-21 by

    University of Alaska Anchorage, College of Engineering.

    All rights reserved.

    Contributors:  ...
		   ...                
		   Christoph Lauter and James Flemings, Luka Spaic

    See file memory.c on how to compile this code.

    Implement the functions __malloc_impl, __calloc_impl,
    __realloc_impl and __free_impl below. The functions must behave
    like malloc, calloc, realloc and free but your implementation must
    of course not be based on malloc, calloc, realloc and free.

    Use the mmap and munmap system calls to create private anonymous
    memory mappings and hence to get basic access to memory, as the
    kernel provides it. Implement your memory management functions
    based on that "raw" access to user space memory.

    As the mmap and munmap system calls are slow, you have to find a
    way to reduce the number of system calls, by "grouping" them into
    larger blocks of memory accesses. As a matter of course, this needs
    to be done sensibly, i.e. without wasting too much memory.

    You must not use any functions provided by the system besides mmap
    and munmap. If you need memset and memcpy, use the naive
    implementations below. If they are too slow for your purpose,
    rewrite them in order to improve them!

    Catch all errors that may occur for mmap and munmap. In these cases
    make malloc/calloc/realloc/free just fail. Do not print out any 
    debug messages as this might get you into an infinite recursion!

    Your __calloc_impl will probably just call your __malloc_impl, check
    if that allocation worked and then set the fresh allocated memory
    to all zeros. Be aware that calloc comes with two size_t arguments
    and that malloc has only one. The classical multiplication of the two
    size_t arguments of calloc is wrong! Read this to convince yourself:

    https://bugzilla.redhat.com/show_bug.cgi?id=853906

    In order to allow you to properly refuse to perform the calloc instead
    of allocating too little memory, the __try_size_t_multiply function is
    provided below for your convenience.
    
*/

#include <stddef.h>
#include <sys/mman.h>

/* Predefined helper functions */

static void *__memset(void *s, int c, size_t n) {
  unsigned char *p;
  size_t i;

  if (n == ((size_t) 0)) return s;
  for (i=(size_t) 0,p=(unsigned char *)s;
       i<=(n-((size_t) 1));
       i++,p++) {
    *p = (unsigned char) c;
  }
  return s;
}

static void *__memcpy(void *dest, const void *src, size_t n) {
  unsigned char *pd;
  const unsigned char *ps;
  size_t i;

  if (n == ((size_t) 0)) return dest;
  for (i=(size_t) 0,pd=(unsigned char *)dest,ps=(const unsigned char *)src;
       i<=(n-((size_t) 1));
       i++,pd++,ps++) {
    *pd = *ps;
  }
  return dest;
}

/* Tries to multiply the two size_t arguments a and b.

   If the product holds on a size_t variable, sets the 
   variable pointed to by c to that product and returns a 
   non-zero value.
   
   Otherwise, does not touch the variable pointed to by c and 
   returns zero.

   This implementation is kind of naive as it uses a division.
   If performance is an issue, try to speed it up by avoiding 
   the division while making sure that it still does the right 
   thing (which is hard to prove).

*/
static int __try_size_t_multiply(size_t *c, size_t a, size_t b) {
  size_t t, r, q;

  /* If any of the arguments a and b is zero, everthing works just fine. */
  if ((a == ((size_t) 0)) ||
      (b == ((size_t) 0))) {
    *c = a * b;
    return 1;
  }

  /* Here, neither a nor b is zero. 

     We perform the multiplication, which may overflow, i.e. present
     some modulo-behavior.

  */
  t = a * b;

  /* Perform Euclidian division on t by a:

     t = a * q + r

     As we are sure that a is non-zero, we are sure
     that we will not divide by zero.

  */
  q = t / a;
  r = t % a;

  /* If the rest r is non-zero, the multiplication overflowed. */
  if (r != ((size_t) 0)) return 0;

  /* Here the rest r is zero, so we are sure that t = a * q.

     If q is different from b, the multiplication overflowed.
     Otherwise we are sure that t = a * b.

  */
  if (q != b) return 0;
  *c = t;
  return 1;
}

/* End of predefined helper functions */

/* Your helper functions 

   You may also put some struct definitions, typedefs and global
   variables here. Typically, the instructor's solution starts with
   defining a certain struct, a typedef and a global variable holding
   the start of a linked list of currently free memory blocks. That 
   list probably needs to be kept ordered by ascending addresses.

*/

#define MMAP_MIN_SIZE	((size_t) 16777216) // 16 MB

typedef struct struct_block_t{
	void *addr;
	size_t length;
	size_t mmap_size;
	struct struct_block_t *next;
    int free;
} block_t; 

#define MEM_SIZE	(sizeof(block_t))

static block_t *head;

void remove_block(block_t *ptr){
    /*
     * This function takes in a pointer (ptr) to a block and either
     * 1.) merges with the left or right pointers 
     * 2.) becomes unmapped
     */
    block_t *prev_prev, *prev, *cur;
    prev_prev = NULL; // previous node of the previous node
    prev = NULL; 
    cur = head;

    while (cur != ptr){
        prev_prev = prev;
        prev = cur;
        cur = cur->next;
        if (cur == NULL)
            return;
    }
    // merge with the next pointer of ptr if it is free and in the same block of
    // memory as ptr
    if (ptr->next != NULL && ptr->next->free && ptr->addr == ptr->next->addr){
        ptr->length += ptr->next->length;
        ptr->next = ptr->next->next;
    }
    // merge with with the previous pointer if it meets the same criteria as above
    if (prev != NULL && prev->free && ptr->addr == prev->addr) {
        size_t length = ptr->length;
        block_t *next = ptr->next;
        ptr = prev;
        ptr->length += length;
        ptr->next = next;
    }

    ptr->free = 1;
    if (ptr->length != ptr->mmap_size) // not all memory in current block is free
        return;
    
    // all memory in current block is free, so unmap 
    if (ptr == head){
        if (ptr->next == NULL){
            if (munmap(ptr->addr, ptr->mmap_size) == 0){
                head = NULL;
            }
        }
        else if (ptr->next->addr != ptr->addr){
            block_t *next = ptr->next;
            if (munmap(ptr->addr, ptr->mmap_size) == 0){
                head = next;
            }
        }
    }
    else if (ptr->next != NULL && ptr->next->addr != ptr->addr){
        if (munmap(ptr->addr, ptr->mmap_size) == 0){
            if (ptr != prev)
                prev_prev->next = ptr->next;
            else
                prev->next = ptr->next;
        }
    }

	return;
}

void *split_block(block_t *new, size_t size){
    /*
     * split the block of memory into two pieces:
     * 1.) left side is the pointer new with length=size.
     * 2.) new pointer with its length equal to remaining available memory
     */
    block_t *nxt_new;
    nxt_new = (block_t *) (((void *) new) + size);
    nxt_new->length = new->length - size;
    nxt_new->mmap_size = new->mmap_size;
    nxt_new->addr = new->addr;
    nxt_new->next = new->next;
    nxt_new->free = 1;

    new->next = nxt_new;
    return NULL;
}

block_t *get_block(size_t raw_size){
    /*
     * find a pointer in a block of memory that:
     * 1.) has enough length to cover the requested size + MEM_SIZE
     * 2.) is free
     */
	block_t *cur; 
    size_t size;
	if (head == NULL) return NULL; // no memory available
    if (raw_size == 0) return NULL; 

    size = raw_size + MEM_SIZE; 
    if (size < raw_size) return NULL; // in case of overflow

    cur = head;
    while (cur->length < size || !cur->free){ 
        cur = cur->next;
        if (cur == NULL)
            return NULL;
    }
    
    cur->free = 0;
    // is there enough memory available in the block that cur is on
    // to split the block further?
    if ((cur->length - size) > MEM_SIZE){ 
        split_block(cur, size);
    } 

    cur->length = size;    
	return cur;
}

void *add_block(block_t *new){
    /*
     * a new block of memory (new) was created, so add it to the 
     * linked list of blocks. it is sorted in ascending order by memory address
     */
    block_t *prev, *cur;
	cur = head;
    prev = NULL;

	while (cur != NULL){
        if (new->addr < cur->addr) 
            break;
		prev = cur;
		cur = cur->next;
	}

    if (prev == NULL) { // cur == head
        head = new;
        new->next = cur;
    }

    else{ 
        prev->next = new;
        new->next = cur;
    }

    return NULL;
}

void *new_block(size_t raw_size){
    /*
     * generate a new block of memory by using mmap
     */
    void *ptr;
	block_t *new;
	size_t length, size;
    if (raw_size == 0) return NULL;
    size = raw_size + MEM_SIZE;
    length = MMAP_MIN_SIZE;
	if (size > length) // if size is greater than min map, then set length = size
		length = size;

	ptr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (ptr == MAP_FAILED) return NULL;

    new = (block_t *) ptr;
	new->length = length;
	new->mmap_size = length;
	new->addr = ptr; 
    new->free = 1;
    new->next = NULL;

    add_block(new); // add block to linked list
    return NULL;
}

/* End of your helper functions */

/* Start of the actual malloc/calloc/realloc/free functions */

void __free_impl(void *);

void *__malloc_impl(size_t size) {
	size_t s;
	void *ptr;

	if (size == ((size_t) 0)) return NULL;

	s = size + MEM_SIZE;
	if (s < size) return NULL;

	ptr = (void *) get_block(s);  

	if (ptr != NULL)
		return ptr + MEM_SIZE;

    new_block(s);

	ptr = (void *) get_block(s);  

	if (ptr != NULL)
		return ptr + MEM_SIZE;

  	return NULL;
}

void *__calloc_impl(size_t nmemb, size_t size) {
  size_t s;
  void *ptr;

    if (!__try_size_t_multiply(&s, nmemb, size)){
        return NULL;
    }
    ptr = __malloc_impl(s); 
    if (ptr != NULL){
        __memset(ptr, 0, s);
    }
    return ptr;
}

void *__realloc_impl(void *ptr, size_t size) {
    void *new_ptr;
    block_t *old_block;
    size_t s;

    if (ptr == NULL) return __malloc_impl(size);

    if (size == ((size_t) 0)){
        __free_impl(ptr);
        return NULL;
    }
    new_ptr = __malloc_impl(size);

    if (new_ptr == NULL ) return NULL;

    old_block = (block_t *) (ptr - MEM_SIZE);
    s = old_block->length;

    if (size < s){
      s = size;
    }

    __memcpy(new_ptr, ptr, s);
    __free_impl(ptr);
    return new_ptr;
}

void __free_impl(void *ptr) {
    if (ptr != NULL)
        remove_block(ptr - MEM_SIZE);
    return;
}

/* End of the actual malloc/calloc/realloc/free functions */
