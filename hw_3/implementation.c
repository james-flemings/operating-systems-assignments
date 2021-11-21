/*

  MyFS: a tiny file-system written for educational purposes

  MyFS is 

  Copyright 2018-21 by

  University of Alaska Anchorage, College of Engineering.

  Contributors: Christoph Lauter
                ... and
                ... James Flemings, Luka Spaic

  and based on 

  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs

*/

#include <stddef.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>


/* The filesystem you implement must support all the 13 operations
   stubbed out below. There need not be support for access rights,
   links, symbolic links. There needs to be support for access and
   modification times and information for statfs.

   The filesystem must run in memory, using the memory of size 
   fssize pointed to by fsptr. The memory comes from mmap and 
   is backed with a file if a backup-file is indicated. When
   the filesystem is unmounted, the memory is written back to 
   that backup-file. When the filesystem is mounted again from
   the backup-file, the same memory appears at the newly mapped
   in virtual address. The filesystem datastructures hence must not
   store any pointer directly to the memory pointed to by fsptr; it
   must rather store offsets from the beginning of the memory region.

   When a filesystem is mounted for the first time, the whole memory
   region of size fssize pointed to by fsptr reads as zero-bytes. When
   a backup-file is used and the filesystem is mounted again, certain
   parts of the memory, which have previously been written, may read
   as non-zero bytes. The size of the memory region is at least 2048
   bytes.

   CAUTION:

   * You MUST NOT use any global variables in your program for reasons
   due to the way FUSE is designed.

   You can find ways to store a structure containing all "global" data
   at the start of the memory region representing the filesystem.

   * You MUST NOT store (the value of) pointers into the memory region
   that represents the filesystem. Pointers are virtual memory
   addresses and these addresses are ephemeral. Everything will seem
   okay UNTIL you remount the filesystem again.

   You may store offsets/indices (of type size_t) into the
   filesystem. These offsets/indices are like pointers: instead of
   storing the pointer, you store how far it is away from the start of
   the memory region. You may want to define a type for your offsets
   and to write two functions that can convert from pointers to
   offsets and vice versa.

   * You may use any function out of libc for your filesystem,
   including (but not limited to) malloc, calloc, free, strdup,
   strlen, strncpy, strchr, strrchr, memset, memcpy. However, your
   filesystem MUST NOT depend on memory outside of the filesystem
   memory region. Only this part of the virtual memory address space
   gets saved into the backup-file. As a matter of course, your FUSE
   process, which implements the filesystem, MUST NOT leak memory: be
   careful in particular not to leak tiny amounts of memory that
   accumulate over time. In a working setup, a FUSE process is
   supposed to run for a long time!

   It is possible to check for memory leaks by running the FUSE
   process inside valgrind:

   valgrind --leak-check=full ./myfs --backupfile=test.myfs ~/fuse-mnt/ -f

   However, the analysis of the leak indications displayed by valgrind
   is difficult as libfuse contains some small memory leaks (which do
   not accumulate over time). We cannot (easily) fix these memory
   leaks inside libfuse.

   * Avoid putting debug messages into the code. You may use fprintf
   for debugging purposes but they should all go away in the final
   version of the code. Using gdb is more professional, though.

   * You MUST NOT fail with exit(1) in case of an error. All the
   functions you have to implement have ways to indicated failure
   cases. Use these, mapping your internal errors intelligently onto
   the POSIX error conditions.

   * And of course: your code MUST NOT SEGFAULT!

   It is reasonable to proceed in the following order:

   (1)   Design and implement a mechanism that initializes a filesystem
         whenever the memory space is fresh. That mechanism can be
         implemented in the form of a filesystem handle into which the
         filesystem raw memory pointer and sizes are translated.
         Check that the filesystem does not get reinitialized at mount
         time if you initialized it once and unmounted it but that all
         pieces of information (in the handle) get read back correctly
         from the backup-file. 

   (2)   Design and implement functions to find and allocate free memory
         regions inside the filesystem memory space. There need to be 
         functions to free these regions again, too. Any "global" variable
         goes into the handle structure the mechanism designed at step (1) 
         provides.

   (3)   Carefully design a data structure able to represent all the
         pieces of information that are needed for files and
         (sub-)directories.  You need to store the location of the
         root directory in a "global" variable that, again, goes into the 
         handle designed at step (1).
          
   (4)   Write __myfs_getattr_implem and debug it thoroughly, as best as
         you can with a filesystem that is reduced to one
         function. Writing this function will make you write helper
         functions to traverse paths, following the appropriate
         subdirectories inside the file system. Strive for modularity for
         these filesystem traversal functions.

   (5)   Design and implement __myfs_readdir_implem. You cannot test it
         besides by listing your root directory with ls -la and looking
         at the date of last access/modification of the directory (.). 
         Be sure to understand the signature of that function and use
         caution not to provoke segfaults nor to leak memory.

   (6)   Design and implement __myfs_mknod_implem. You can now touch files 
         with 

         touch foo

         and check that they start to exist (with the appropriate
         access/modification times) with ls -la.

   (7)   Design and implement __myfs_mkdir_implem. Test as above.

   (8)   Design and implement __myfs_truncate_implem. You can now 
         create files filled with zeros:

         truncate -s 1024 foo

   (9)   Design and implement __myfs_statfs_implem. Test by running
         df before and after the truncation of a file to various lengths. 
         The free "disk" space must change accordingly.

   (10)  Design, implement and test __myfs_utimens_implem. You can now 
         touch files at different dates (in the past, in the future).

   (11)  Design and implement __myfs_open_implem. The function can 
         only be tested once __myfs_read_implem and __myfs_write_implem are
         implemented.

   (12)  Design, implement and test __myfs_read_implem and
         __myfs_write_implem. You can now write to files and read the data 
         back:

         echo "Hello world" > foo
         echo "Hallo ihr da" >> foo
         cat foo

         Be sure to test the case when you unmount and remount the
         filesystem: the files must still be there, contain the same
         information and have the same access and/or modification
         times.

   (13)  Design, implement and test __myfs_unlink_implem. You can now
         remove files.

   (14)  Design, implement and test __myfs_unlink_implem. You can now
         remove directories.

   (15)  Design, implement and test __myfs_rename_implem. This function
         is extremely complicated to implement. Be sure to cover all 
         cases that are documented in man 2 rename. The case when the 
         new path exists already is really hard to implement. Be sure to 
         never leave the filessystem in a bad state! Test thoroughly 
         using mv on (filled and empty) directories and files onto 
         inexistant and already existing directories and files.

   (16)  Design, implement and test any function that your instructor
         might have left out from this list. There are 13 functions 
         __myfs_XXX_implem you have to write.

   (17)  Go over all functions again, testing them one-by-one, trying
         to exercise all special conditions (error conditions): set
         breakpoints in gdb and use a sequence of bash commands inside
         your mounted filesystem to trigger these special cases. Be
         sure to cover all funny cases that arise when the filesystem
         is full but files are supposed to get written to or truncated
         to longer length. There must not be any segfault; the user
         space program using your filesystem just has to report an
         error. Also be sure to unmount and remount your filesystem,
         in order to be sure that it contents do not change by
         unmounting and remounting. Try to mount two of your
         filesystems at different places and copy and move (rename!)
         (heavy) files (your favorite movie or song, an image of a cat
         etc.) from one mount-point to the other. None of the two FUSE
         processes must provoke errors. Find ways to test the case
         when files have holes as the process that wrote them seeked
         beyond the end of the file several times. Your filesystem must
         support these operations at least by making the holes explicit 
         zeros (use dd to test this aspect).

   (18)  Run some heavy testing: copy your favorite movie into your
         filesystem and try to watch it out of the filesystem.

*/

/* Helper types and functions */

/* YOUR HELPER FUNCTIONS GO HERE */

#define MAX_FILE_NAME ((size_t) 256)
#define MAGIC_NUM ((size_t) 1)
#define MIN_SIZE ((size_t) 4096)

typedef size_t offset_t;

typedef struct memory_block {
    size_t size; // usable memory
    size_t allocated;
    offset_t nxt_block; // to data_block
} memory_block_t;

typedef enum inode_enum_type inode_type_t;
enum inode_enum_type {
    DIRECTORY,
    REG_FILE
};

typedef struct inode_struct_file{
   size_t size; 
   offset_t first_block; // to file_block
} inode_file_t;

typedef struct file_block {
    size_t block_size; 
    offset_t nxt_file_block; // next file block
    offset_t data; // to data_block
} file_block_t;

typedef struct inode_struct_dir{
    size_t num_children;
    offset_t children;  
} inode_dir_t;

static inline offset_t ptr_to_offset(void *ptr, void *fstpr){
    if (ptr < fstpr) return 0;
    return (offset_t) (ptr - fstpr);
}

static inline void *offset_to_ptr(void *ptr, offset_t offset){
    if (offset == 0) return NULL;
    return (void *) (ptr + offset);
}

/*
 * Contains metadata about a file
 */
typedef struct inode {
    char name[MAX_FILE_NAME];
    struct timespec mod_time;
    struct timespec acc_time;
    inode_type_t type;
    union {
        inode_file_t file;  
        inode_dir_t directory;
    } value;
} inode_t;

typedef struct super_block {
    uint32_t magic;
    size_t size;
    offset_t free_memory;
    offset_t root_dir;
} super_block_t;

#define SUPER_BLOCK_SIZE ((size_t) sizeof(super_block_t))
#define MEM_BLOCK_SIZE ((size_t) sizeof(memory_block_t))
#define INODE_SIZE ((size_t) sizeof(inode_t))
#define FILE_BLOCK_SIZE ((size_t) sizeof(file_block_t))

super_block_t *get_handle(void *fsptr, size_t size){
    super_block_t *handle = (super_block_t*) fsptr;
    memory_block_t *block;

    if (size < SUPER_BLOCK_SIZE) return NULL;

    if (handle->magic != MAGIC_NUM){
        size_t s = size - SUPER_BLOCK_SIZE;  
        memset(fsptr + SUPER_BLOCK_SIZE, 0, s);
        handle->magic = MAGIC_NUM; 
        handle->size = s;

        if (s == (size_t) 0)
            handle->free_memory = (offset_t) 0;

        else{
            block = (memory_block_t *) offset_to_ptr(fsptr, SUPER_BLOCK_SIZE);
            block->size = s;
            block->nxt_block = (offset_t) 0;
            handle->free_memory = ptr_to_offset(block, fsptr);
        }           

        handle->root_dir =(offset_t) 0;
    }
     
    return handle;
}

size_t free_size(super_block_t *handle){
    size_t total_free_size;
    memory_block_t *block;

    total_free_size = (size_t) 0;
    for (block = (memory_block_t *) offset_to_ptr(handle, handle->free_memory);
            block != NULL; block = (memory_block_t *) (offset_to_ptr(handle,
                    block->nxt_block))){
        total_free_size += block->size; 
    }

    return total_free_size;
}

memory_block_t *get_memory_block(super_block_t *handle, size_t size){
    memory_block_t *cur, *prev, *next;
    for (cur = (memory_block_t *) offset_to_ptr(handle, handle->free_memory),
         prev = NULL; cur != NULL && cur->nxt_block != (offset_t) 0; prev = cur,
         cur=(memory_block_t *) offset_to_ptr(handle, cur->nxt_block)){
        
        if (cur->size > size)
            break;
    }

    // there does not exist a block with enough size
    if (cur->size < size){
        return NULL;
    }

    if (cur->size - size > (size_t) 0){ // create new next block
        //if ((cur->size - size) < MEM_BLOCK_SIZE){ // not enough memory to create
        //    next = (memory_block_t *) handle;
        //}

        //else{
        next = (memory_block_t *) offset_to_ptr(cur, size);
        next->size = cur->size - size;
        next->nxt_block = cur->nxt_block;
        //}
    }

    else { // already exists a next block
        next = (memory_block_t *) offset_to_ptr(handle, cur->nxt_block);
    }

    // cur is first available memory block
    if (cur == (memory_block_t *) offset_to_ptr(handle, handle->free_memory))
        handle->free_memory = ptr_to_offset(next, handle);
    else
        prev->nxt_block = ptr_to_offset(next, handle);

    cur->size = size; 
    cur->nxt_block = (offset_t) 0;

    return cur;
}

// add block back to free memory linked list
void add_to_free_memory(super_block_t *handle, offset_t offset){
    memory_block_t *block, *cur, *prev;
    block = (memory_block_t *) offset_to_ptr(handle, offset);
    for (cur = (memory_block_t *) offset_to_ptr(handle, handle->free_memory), 
                prev = NULL; cur != NULL; prev = cur,
                cur= (memory_block_t *) offset_to_ptr(handle, cur->nxt_block)){

        if ((void *) block < (void *) cur)
            break;
    }

    // place block in between prev and cur block
    if (cur != NULL){
        block->nxt_block = ptr_to_offset(cur, handle);
    }
    else {
        block->nxt_block = (offset_t) 0;
    }

    if (prev == NULL) // block is new head
       handle->free_memory = offset;      

    else{
        prev->nxt_block = offset;
    }
    
    // merge with right block
    if (cur != NULL && ((void *) ((void *) block + block->size)) == ((void *) cur)){
        block->size += cur->size;
        block->nxt_block = cur->nxt_block;
    }

    // merge with left block
    if (prev != NULL && ((void *) ((void *) prev + prev->size)) == ((void *) block)){
        prev->size += block->size;
        prev->nxt_block = block->nxt_block;
    }

    return;
}

void free_memory(super_block_t *handle, offset_t offset){
    void *ptr = ((void *) offset_to_ptr(handle, offset)) - MEM_BLOCK_SIZE; 
    offset_t new_offset = ptr_to_offset(ptr, handle);
    add_to_free_memory(handle, new_offset);
}

offset_t allocate_memory(super_block_t *handle, size_t size){
    size_t s;
    void *ptr;

    if (size == ((size_t) 0)) return (offset_t) 0;

    s = size + MEM_BLOCK_SIZE;
    if (s < size) return (offset_t) 0;

    ptr = (void *) get_memory_block(handle, s);

    if (ptr != NULL)
        return ptr_to_offset((ptr + MEM_BLOCK_SIZE), handle);

    return (offset_t) 0;
}

offset_t reallocate_memory(super_block_t *handle, offset_t offset, size_t size){
    size_t s;
    void *old_ptr, *new_block;
    memory_block_t *old_block;
    offset_t newOffset;

    if (handle == NULL) return (offset_t) 0;

    if (offset == (offset_t) 0) return (offset_t) 0;

    if (size == (size_t) 0){
        free_memory(handle, offset);
        return (offset_t) 0;
    }

    newOffset = allocate_memory(handle, size);
    if (newOffset == (offset_t) 0) return (offset_t) 0;  

    old_ptr = offset_to_ptr(handle, offset);
    old_block = (memory_block_t *) (old_ptr - MEM_BLOCK_SIZE);

    s = old_block->size;
    if (size < s)
        s = size;

    new_block = offset_to_ptr(handle, newOffset);
    memcpy(new_block, old_ptr, s);
    free_memory(handle, offset);
    return newOffset;
}

inode_t *get_path(super_block_t *handle, const char *path){
    inode_t *node, *child;
    char *index, *__path, *name, file_name[MAX_FILE_NAME];
    size_t size;

    if (handle->root_dir == (offset_t) 0){
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        handle->root_dir = allocate_memory(handle, INODE_SIZE);
        inode_t *root = (inode_t *) offset_to_ptr(handle, handle->root_dir);
        root->name[0] = '/';
        root->name[1] = '\0';
        root->type = DIRECTORY;
        root->mod_time = ts;
        root->acc_time = ts;

        root->value.directory.num_children = (size_t) 0;
        root->value.directory.children = (offset_t) 0;
    }

    node = (inode_t *) offset_to_ptr(handle, handle->root_dir);

    if (strcmp("/\0", path) == 0){ // path is root directory
        return node;
    }
    
    __path = (char *) malloc((strlen(path)+1) * sizeof(char));
    if (__path == NULL){
        return NULL;
    }
    strcpy(__path, path);
    name = __path + 1;
    index = strchr(name, '/');

    while (strlen(name) != 0){
        child = NULL; 

        if (index != NULL)
            size = (size_t) (((void *) index) - ((void *) name));

        else
            size = (size_t) strlen(name);

        strncpy(file_name, name, size);
        file_name[size] = '\0';

        for (size_t i = 0; i < node->value.directory.num_children; i++){
            child = (inode_t *) offset_to_ptr(handle,
                        (node->value.directory.children + i*INODE_SIZE));

            if (strcmp(child->name, file_name) == 0){
                node = child;
                break;
            }
        }
        
        memset(file_name, 0, size);
        if (node != child) { // path not found
            free(__path);
            return NULL;
        }

        if (index == NULL) {
            break;
        }

        name = index + 1;
        index = strchr(name, '/');
    }
    
    free(__path);
    return node;
}

size_t max_size(super_block_t *handle){
    size_t max_free_size;
    memory_block_t *block;

    max_free_size = (size_t) 0;
    for (block = (memory_block_t *) offset_to_ptr(handle, handle->free_memory);
            block != NULL; block = (memory_block_t *) (offset_to_ptr(handle,
                    block->nxt_block))){
        if (block->size > max_free_size)
            max_free_size = block->size;
    }

    return max_free_size;
}

/* End of helper functions */

/* Implements an emulation of the stat system call on the filesystem 
   of size fssize pointed to by fsptr. 
   
   If path can be followed and describes a file or directory 
   that exists and is accessable, the access information is 
   put into stbuf.  On success, 0 is returned. On failure, -1 is returned and the appropriate error code is put into *errnoptr.

   man 2 stat documents all possible error codes and gives more detail
   on what fields of stbuf need to be filled in. Essentially, only the
   following fields need to be supported:

   st_uid      the value passed in argument
   st_gid      the value passed in argument
   st_mode     (as fixed values S_IFDIR | 0755 for directories,
                                S_IFREG | 0755 for files)
   st_nlink    (as many as there are subdirectories (not files) for directories
                (including . and ..),
                1 for files)
   st_size     (supported only for files, where it is the real file size)
   st_atim
   st_mtim

*/
int __myfs_getattr_implem(void *fsptr, size_t fssize, int *errnoptr,
                          uid_t uid, gid_t gid,
                          const char *path, struct stat *stbuf) {

    super_block_t *handle; 
    inode_t *node;
    char *file_name;

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    //printf("GETATTR %s\n", path);

    file_name = strrchr(path, '/') + 1;
    if (strlen(file_name) >= MAX_FILE_NAME){
        *errnoptr = ENAMETOOLONG;
        return -1;
    }

    node = get_path(handle, path);
    if (node == NULL){
        *errnoptr = ENOENT;
        return -1;
    }

    memset(stbuf, 0, sizeof(struct stat));

    stbuf->st_uid = uid;
    stbuf->st_gid = gid;
    stbuf->st_atim = node->acc_time;
    stbuf->st_mtim = node->mod_time;

    if (node->type == DIRECTORY){
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = node->value.directory.num_children + ((size_t) 2);
    }
    else{
        stbuf->st_mode = S_IFREG | 0755;
        stbuf->st_size = node->value.file.size;
    }

    return 0;
}

/* Implements an emulation of the readdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   If path can be followed and describes a directory that exists and
   is accessable, the names of the subdirectories and files 
   contained in that directory are output into *namesptr. The . and ..
   directories must not be included in that listing.

   If it needs to output file and subdirectory names, the function
   starts by allocating (with calloc) an array of pointers to
   characters of the right size (n entries for n names). Sets
   *namesptr to that pointer. It then goes over all entries
   in that array and allocates, for each of them an array of
   characters of the right size (to hold the i-th name, together 
   with the appropriate '\0' terminator). It puts the pointer
   into that i-th array entry and fills the allocated array
   of characters with the appropriate name. The calling function
   will call free on each of the entries of *namesptr and 
   on *namesptr.

   The function returns the number of names that have been 
   put into namesptr. 

   If no name needs to be reported because the directory does
   not contain any file or subdirectory besides . and .., 0 is 
   returned and no allocation takes place.

   On failure, -1 is returned and the *errnoptr is set to 
   the appropriate error code. 

   The error codes are documented in man 2 readdir.

   In the case memory allocation with malloc/calloc fails, failure is
   indicated by returning -1 and setting *errnoptr to EINVAL.

*/
int __myfs_readdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, char ***namesptr) {

    super_block_t *handle;
    inode_t *node, *child;
    char **names;
    size_t size;

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    //printf("READDIR %s\n", path);
    node = get_path(handle, path);
    if (node == NULL){
        *errnoptr = ENOENT;
        return -1;
    }

    size = node->value.directory.num_children;
    if (size == (size_t) 0){
        return 0;
    }

    names = (char **) calloc(size, sizeof(char *));
    if (names == NULL){
        *errnoptr = ENOMEM;
        return -1;
    }
    
    for (size_t i = 0; i < size; i++){
        child = ((inode_t *) offset_to_ptr(handle,
                    (node->value.directory.children + i*INODE_SIZE)));

        names[i] = (char *) calloc(strlen(child->name), sizeof(char));
        strcpy(names[i], child->name);
    }

    *namesptr = names;
    return size;
}

/* Implements an emulation of the mknod system call for regular files
   on the filesystem of size fssize pointed to by fsptr.

   This function is called only for the creation of regular files.

   If a file gets created, it is of size zero and has default
   ownership and mode bits.

   The call creates the file indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mknod.

*/
int __myfs_mknod_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {

    super_block_t *handle;
    inode_t *node, *child;
    char *file_name, *dir_path;
    size_t dir_len, num_children;
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    //printf("MKNOD %s\n", path);

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    if (INODE_SIZE > max_size(handle)){
        *errnoptr = ENOMEM;
        return -1;
    }

    file_name = strrchr(path, '/') + 1;
    dir_len = strlen(path) - strlen(file_name);
    if (strlen(file_name) >= MAX_FILE_NAME){
        *errnoptr = ENAMETOOLONG;
        return -1;
    }
    
    dir_path = (char *) malloc((dir_len+1) * sizeof(char));
    if (dir_path == NULL){
        *errnoptr = ENOMEM;
        return -1;
    }

    strncpy(dir_path, path, dir_len);
    dir_path[dir_len] = '\0';
    
    node = get_path(handle, dir_path);
    if (node == NULL){
        free(dir_path);
        *errnoptr = ENOENT;
        return -1;
    }

    node->value.directory.num_children++;
    num_children = node->value.directory.num_children;

    if (num_children == 1) {
       node->value.directory.children = allocate_memory(handle, INODE_SIZE);
        if (node->value.directory.children == (offset_t) 0){
            *errnoptr = ENOMEM;
            return -1;
        }
    }

    else{
       node->value.directory.children = reallocate_memory(handle,
                node->value.directory.children, num_children * INODE_SIZE);
        if (node->value.directory.children == (offset_t) 0){
            *errnoptr = ENOMEM;
            return -1;
        }
    }

    child = (inode_t *) offset_to_ptr(handle, (node->value.directory.children 
                + (num_children-1) * INODE_SIZE)); 

    strcpy(child->name, file_name);
    //printf("Child name %s\n", child->name);
    child->type = REG_FILE;
    child->mod_time = ts;
    child->acc_time = ts;
    child->value.file.size = (size_t) 0;
    child->value.file.first_block = (offset_t) 0;

    free(dir_path);
    return 0;
}

/* Implements an emulation of the unlink system call for regular files
   on the filesystem of size fssize pointed to by fsptr.

   This function is called only for the deletion of regular files.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 unlink.

*/
int __myfs_unlink_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {

    super_block_t *handle;
    inode_t *file_node, *dir_node, *node;
    char *file_name, *dir_path;
    size_t dir_len;
    file_block_t *prev, *file_block;

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    //printf("UNLINK %s\n", path);

    file_node = get_path(handle, path);
    if (file_node == NULL){
        *errnoptr = ENOENT;
        return -1;
    }

    if (file_node->type == DIRECTORY){
        *errnoptr = EISDIR;
        return -1;
    }

    file_name = strrchr(path, '/') + 1;
    dir_len = strlen(path) - strlen(file_name);

    dir_path = (char *) malloc((dir_len+1) * sizeof(char));
    if (dir_path == NULL){
        *errnoptr = ENOMEM;
        return -1;
    }
    strncpy(dir_path, path, dir_len);
    dir_path[dir_len] = '\0';
    
    dir_node = get_path(handle, dir_path);
    if (dir_node == NULL){
        free(dir_path);
        *errnoptr = ENOENT;
        return -1;
    }

    for (size_t i = 0; i < dir_node->value.directory.num_children; i++){
       node = (inode_t *) offset_to_ptr(handle,
               dir_node->value.directory.children + i * INODE_SIZE);

        if (strcmp(node->name, file_name) == 0){
            break;
        }
    }

    for (file_block = (file_block_t *) offset_to_ptr(handle,
                node->value.file.first_block), prev = NULL; file_block != NULL;
            prev = file_block, file_block = (file_block_t *) offset_to_ptr(handle,
                file_block->nxt_file_block)){
        
        //free_memory(handle, file_block->data);
        if (file_block->data != (offset_t) 0)
            file_block->data = reallocate_memory(handle,
                    file_block->data, (size_t) 0);

        if (prev != NULL){
            //free_memory(handle, ptr_to_offset((void *) prev, handle));
            prev = (file_block_t *) reallocate_memory(handle,
                    ptr_to_offset((void *) prev, handle), (size_t) 0);
        } 
    }

    if (prev != NULL){
        //free_memory(handle, ptr_to_offset((void *) prev, handle));
        prev = (file_block_t *) reallocate_memory(handle,
                ptr_to_offset((void *) prev, handle), (size_t) 0);
    }

    node->value.file.size = (size_t) 0;

    if (dir_node->value.directory.num_children > 1) {
        memcpy((void *) node, offset_to_ptr(handle, (dir_node->value.directory.children 
                + (dir_node->value.directory.num_children - 1) * INODE_SIZE)),
            INODE_SIZE);
    }

    dir_node->value.directory.num_children--;
    dir_node->value.directory.children = reallocate_memory(handle,
            dir_node->value.directory.children, (dir_node->value.directory.num_children
                * INODE_SIZE));

    free(dir_path);
    return 0;
}

/* Implements an emulation of the rmdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call deletes the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The function call must fail when the directory indicated by path is
   not empty (if there are files or subdirectories other than . and ..).

   The error codes are documented in man 2 rmdir.

*/
int __myfs_rmdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {

    super_block_t *handle;
    inode_t *file_node, *dir_node, *node;
    char *dir_name, *dir_path;
    size_t dir_len;

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

   // printf("RMDIR %s\n", path);

    file_node = get_path(handle, path);
    if (file_node == NULL){
        *errnoptr = ENOENT;
        return -1;
    }

    if (file_node->value.directory.num_children != 0){
        *errnoptr = ENOTEMPTY;
        return -1;
    }

    dir_name = strrchr(path, '/') + 1;
    dir_len = strlen(path) - strlen(dir_name);

    dir_path = (char *) malloc((dir_len+1) * sizeof(char));
    if (dir_path == NULL){
        *errnoptr = ENOMEM;
        return -1;
    }
    strncpy(dir_path, path, dir_len);
    dir_path[dir_len] = '\0';
    
    dir_node = get_path(handle, dir_path);
    if (dir_node == NULL){
        free(dir_path);
        *errnoptr = ENOENT;
        return -1;
    }


    for (size_t i = 0; i < dir_node->value.directory.num_children; i++){
        node = (inode_t *) offset_to_ptr(handle,
               dir_node->value.directory.children + i * INODE_SIZE);

        if (strcmp(node->name, dir_name) == 0){
            break;
        }
    }

    if (dir_node->value.directory.num_children > 1) {
        memcpy((void *) node, offset_to_ptr(handle, (dir_node->value.directory.children 
                + (dir_node->value.directory.num_children - 1) * INODE_SIZE)),
            INODE_SIZE);
    }

    dir_node->value.directory.num_children--;
    dir_node->value.directory.children = reallocate_memory(handle,
            dir_node->value.directory.children, (dir_node->value.directory.num_children
                * INODE_SIZE));

    free(dir_path);
    return 0;
}

/* Implements an emulation of the mkdir system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call creates the directory indicated by path.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 mkdir.

*/
int __myfs_mkdir_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path) {

    super_block_t *handle;
    inode_t *node, *child, *dir_node;
    char *dir_name, *dir_path;
    size_t dir_len, num_children;
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    //printf("MKDIR %s\n", path);

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    if (INODE_SIZE > max_size(handle)){
        *errnoptr = ENOMEM;
        return -1;
    }

    dir_node = get_path(fsptr, path);
    if (dir_node != NULL){
        *errnoptr = EEXIST;
        return -1;
    }

    dir_name = strrchr(path, '/') + 1;
    dir_len = strlen(path) - strlen(dir_name);
    if (strlen(dir_name) >= MAX_FILE_NAME){
        *errnoptr = ENAMETOOLONG;
        return -1;
    }

    dir_path = malloc((dir_len+1) * sizeof(char));
    if (dir_path == NULL){
        *errnoptr = ENOMEM;
        return -1;
    }
    strncpy(dir_path, path, dir_len);
    dir_path[dir_len] = '\0';
    
    node = get_path(handle, dir_path);
    if (node == NULL){
        free(dir_path);
        *errnoptr = ENOENT;
        return -1;
    }

    node->value.directory.num_children++;
    num_children = node->value.directory.num_children;

    if (num_children == 1) {
       node->value.directory.children = allocate_memory(handle, INODE_SIZE);
        if (node->value.directory.children == (offset_t) 0){
            *errnoptr = ENOMEM;
            return -1;
        }
    }

    else {
       node->value.directory.children = reallocate_memory(handle,
                node->value.directory.children, num_children * INODE_SIZE);
        if (node->value.directory.children == (offset_t) 0){
            *errnoptr = ENOMEM;
            return -1;
        }
    }

    child = (inode_t *) offset_to_ptr(handle, (node->value.directory.children 
                + (num_children-1) * INODE_SIZE)); 

    strcpy(child->name, dir_name);
    child->type = DIRECTORY;
    child->mod_time = ts;
    child->acc_time = ts;
    child->value.directory.num_children = (size_t) 0;
    child->value.directory.children = (offset_t) 0;

    free(dir_path);
    return 0;
}

/* Implements an emulation of the rename system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call moves the file or directory indicated by from to to.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   Caution: the function does more than what is hinted to by its name.
   In cases the from and to paths differ, the file is moved out of 
   the from path and added to the to path.

   The error codes are documented in man 2 rename.

*/
int __myfs_rename_implem(void *fsptr, size_t fssize, int *errnoptr,
                         const char *from, const char *to) {

    super_block_t *handle;
    inode_t *from_file, *from_dir, *to_dir;
    char *from_file_name, *to_file_name, *from_dir_name, *to_dir_name;
    size_t from_dir_len, to_dir_len;
    
    //printf("RENAME %s to %s\n", from, to);
    if (strcmp(from , to) == 0)
        return 0;

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    from_file = get_path(handle, from);
    if (from_file == NULL){
        *errnoptr = ENOENT;
        return -1;
    }

    to_file_name = strrchr(to, '/') + 1;
    to_dir_len = strlen(to) - strlen(to_file_name);
    from_file_name = strrchr(from, '/') + 1;
    from_dir_len = strlen(from) - strlen(from_file_name);

    if (strlen(to_file_name) >= MAX_FILE_NAME){
        *errnoptr = ENAMETOOLONG;
        return -1;
    }

    to_dir_name = (char *) malloc((to_dir_len + 1) * sizeof(char));
    from_dir_name = (char *) malloc((from_dir_len+1) * sizeof(char));
    if (from_dir_name == NULL || to_dir_name == NULL){
        *errnoptr = ENOMEM;
        return -1;
    }

    strncpy(from_dir_name, from, from_dir_len);
    from_dir_name[from_dir_len] = '\0';

    strncpy(to_dir_name, to, to_dir_len);
    to_dir_name[to_dir_len] = '\0';
    
    from_dir = get_path(handle, from_dir_name);
    if (from_dir == NULL){
        free(from_dir_name);
        free(to_dir_name);
        *errnoptr = ENOENT;
        return -1;
    }

    to_dir = get_path(handle, to_dir_name);
    if (to_dir == NULL){
        free(from_dir_name);
        free(to_dir_name);
        *errnoptr = ENOENT;
        return -1;
    }

    strcpy(from_file->name, to_file_name);

    if (strcmp(from_dir_name, to_dir_name) == 0) {
        free(from_dir_name);
        free(to_dir_name);
        return 0;
    }

    to_dir->value.directory.num_children++;
    if (to_dir->value.directory.children == (offset_t) 0){
        to_dir->value.directory.children = allocate_memory(handle,
                (to_dir->value.directory.num_children * INODE_SIZE));
    }

    else{
        to_dir->value.directory.children = reallocate_memory(handle, 
            to_dir->value.directory.children, (to_dir->value.directory.num_children
                * INODE_SIZE));
    }

    // copy file from the "from path" to the "to path"
    memmove(offset_to_ptr(handle, (to_dir->value.directory.children +
                    (to_dir->value.directory.num_children - 1) * INODE_SIZE)),
                (void *) from_file, INODE_SIZE);
      
    // delete the file from the "from path"
    /*
    if (__myfs_unlink_implem(fsptr, fssize, errnoptr, from) == -1)
        return -1;
        */

    from_file->value.file.size = (size_t) 0;
    if (from_dir->value.directory.num_children > 1){
        memmove((void *) from_file, offset_to_ptr(handle,
                    (from_dir->value.directory.children
                    + (from_dir->value.directory.num_children - 1) * INODE_SIZE)),
                 INODE_SIZE);
    }
    
    from_dir->value.directory.num_children--;
    from_dir->value.directory.children = reallocate_memory(handle,
            from_dir->value.directory.children, (from_dir->value.directory.num_children
                * INODE_SIZE));

    free(from_dir_name);
    free(to_dir_name);
    return 0;
}

/* Implements an emulation of the truncate system call on the filesystem 
   of size fssize pointed to by fsptr. 

   The call changes the size of the file indicated by path to offset
   bytes.

   When the file becomes smaller due to the call, the extending bytes are
   removed. When it becomes larger, zeros are appended.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 truncate.

*/
int __myfs_truncate_implem(void *fsptr, size_t fssize, int *errnoptr,
                           const char *path, off_t offset) {

    super_block_t *handle; 
    inode_t *node;
    file_block_t *file_block, *prev;
    off_t new_offset;

    //printf("TRUNCATE %s, offset %ld\n", path, offset);

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    node = get_path(handle, path);
    if (node == NULL){
        *errnoptr = ENOENT;
        return -1;
    }

    if (node->type == DIRECTORY){ // file is a directory
        *errnoptr = EISDIR;
        return -1;
    }
    
    if (node->value.file.size == (size_t) 0){
        if ((offset+FILE_BLOCK_SIZE) > (off_t) max_size(handle)){
            *errnoptr = ENOMEM;
            return -1;
        }

        node->value.file.first_block = allocate_memory(handle, FILE_BLOCK_SIZE);

        if (node->value.file.first_block == (offset_t) 0){
            *errnoptr = ENOMEM;
            return -1;
        }

        file_block = (file_block_t *) offset_to_ptr(handle,
                node->value.file.first_block);
        node->value.file.size = offset;
    }

    else if (offset > node->value.file.size){
        if ((offset+FILE_BLOCK_SIZE) > (off_t) max_size(handle)){
            *errnoptr = ENOMEM;
            return -1;
        }
        for (file_block = (file_block_t *) offset_to_ptr(handle,
                   node->value.file.first_block), prev = NULL; file_block != NULL;
                prev = file_block, file_block = (file_block_t *) offset_to_ptr(handle,
                    file_block->nxt_file_block));
        
        prev->nxt_file_block = allocate_memory(handle, FILE_BLOCK_SIZE);
        if (prev->nxt_file_block == (offset_t) 0){
            *errnoptr = ENOMEM;
            return -1;
        }

        file_block = (file_block_t *) offset_to_ptr(handle, prev->nxt_file_block);
        node->value.file.size = offset;
    } 

    else{
        node->value.file.size = offset;
        new_offset = offset; 
        for (file_block = (file_block_t *) offset_to_ptr(handle,
                   node->value.file.first_block); file_block != NULL;
                   file_block = (file_block_t *) offset_to_ptr(handle,
                   file_block->nxt_file_block)){

            if (new_offset > file_block->block_size)
                new_offset -= file_block->block_size;

            else
                break;
        }

        if (file_block == NULL){
            //printf("This shouldn't happen\n");
            return -1;
        } 
        
        file_block->data = reallocate_memory(handle, file_block->data, new_offset);
        file_block->block_size = new_offset;
        prev = NULL;
        file_block = (file_block_t*) offset_to_ptr(handle, file_block->nxt_file_block);
        while (file_block != NULL){
            if (prev != NULL){
                prev = (file_block_t *) reallocate_memory(handle, ptr_to_offset((void *)
                            prev, handle), (size_t) 0);
            }

            free_memory(handle, file_block->data);
            prev = file_block;
            file_block = (file_block_t*) offset_to_ptr(handle,
                    file_block->nxt_file_block);
        }
            if (prev != NULL){
                free_memory(handle, ptr_to_offset((void *) prev, handle));
            }
        return 0;
    }

    file_block->nxt_file_block = (offset_t) 0;
    file_block->data = allocate_memory(handle, offset);
    if (file_block->data == (offset_t) 0){
        *errnoptr = ENOMEM;
        return -1;
    }
    file_block->block_size = offset;
    memset(offset_to_ptr(handle, file_block->data), '\0', offset);

    return 0;
}

/* Implements an emulation of the open system call on the filesystem 
   of size fssize pointed to by fsptr, without actually performing the opening
   of the file (no file descriptor is returned).

   The call just checks if the file (or directory) indicated by path
   can be accessed, i.e. if the path can be followed to an existing
   object for which the access rights are granted.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The two only interesting error codes are 

   * EFAULT: the filesystem is in a bad state, we can't do anything

   * ENOENT: the file that we are supposed to open doesn't exist (or a
             subpath).

   It is possible to restrict ourselves to only these two error
   conditions. It is also possible to implement more detailed error
   condition answers.

   The error codes are documented in man 2 open.

*/
int __myfs_open_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path) {

    super_block_t *handle; 
    inode_t *node;
    //printf("Open %s\n", path);

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    node = get_path(handle, path);
    if (node == NULL){
        *errnoptr = ENOENT;
        return -1;
    }

    return 0;
}

/* Implements an emulation of the read system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call copies up to size bytes from the file indicated by 
   path into the buffer, starting to read at offset. See the man page
   for read for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes read into the buffer is
   returned. The value zero is returned on an end-of-file condition.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 read.

*/
int __myfs_read_implem(void *fsptr, size_t fssize, int *errnoptr,
                       const char *path, char *buf, size_t size, off_t offset) {

    super_block_t *handle; 
    inode_t *node;
    file_block_t *file_block;
    size_t new_size;
    off_t new_offset;
    int num_bytes = 0;

    //printf("Read %s, size %ld, offset %ld\n", path, size, offset);

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    node = get_path(handle, path);
    if (node == NULL){
        *errnoptr = ENOENT;
        return -1;
    }

    if (node->type == DIRECTORY){
        *errnoptr = EISDIR;
        return -1;
    }

    if ((size_t) offset > node->value.file.size){
        return 0;
    }

    new_size = size;
    new_offset = offset;
    for (file_block = (file_block_t *) offset_to_ptr(handle,
                node->value.file.first_block);
            file_block != NULL;
            file_block = (file_block_t *) offset_to_ptr(handle,
                file_block->nxt_file_block)){

        if (new_offset > file_block->block_size){
            new_offset -= file_block->block_size;
        }
        else
            break;
    }

    while (file_block != NULL){
        if (((size_t) new_offset + new_size) < file_block->block_size){
            memcpy(buf+num_bytes, offset_to_ptr(handle, ((size_t) new_offset
                            + file_block->data)), new_size);
            num_bytes += (int) new_size;
            break;
        }

        else{
            new_size -= (file_block->block_size - new_offset);     
            memcpy(buf+num_bytes, offset_to_ptr(handle, ((size_t) new_offset
                            + file_block->data)), 
                        (file_block->block_size - new_offset));

            num_bytes += (int) (file_block->block_size - new_offset);
        }

        if (new_offset != (off_t) 0)
            new_offset = 0;

        file_block = (file_block_t *) offset_to_ptr(handle, file_block->nxt_file_block);

    }
    return num_bytes;
}

/* Implements an emulation of the write system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call copies up to size bytes to the file indicated by 
   path into the buffer, starting to write at offset. See the man page
   for write for the details when offset is beyond the end of the file etc.
   
   On success, the appropriate number of bytes written into the file is
   returned. The value zero is returned on an end-of-file condition.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 write.

*/
int __myfs_write_implem(void *fsptr, size_t fssize, int *errnoptr,
                        const char *path, const char *buf, size_t size, off_t offset) {
    super_block_t *handle; 
    inode_t *node;
    file_block_t *file_block;
    int num_bytes;

    //printf("Write %s, size %ld, offset %ld\n", path, size, offset);

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    node = get_path(handle, path);
    if (node == NULL){
        *errnoptr = ENOENT;
        return -1;
    }
    
    if (node->type == DIRECTORY){
        *errnoptr = EISDIR;
        return -1;
    }

    if (node->value.file.size == (size_t) 0){
        node->value.file.first_block = allocate_memory(handle, FILE_BLOCK_SIZE);
        
        if (node->value.file.first_block == (size_t) 0){
            *errnoptr = ENOMEM;
            return -1;
        }
    }

    if (offset > node->value.file.size)
        return 0;

    file_block = (file_block_t*) offset_to_ptr(handle, node->value.file.first_block);

    if (node->value.file.size == (size_t) 0){
        file_block->data = allocate_memory(handle, size);
        
        if (file_block->data == (size_t) 0){
            *errnoptr = ENOMEM;
            return -1;
        }
        file_block->block_size = size;
        file_block->nxt_file_block = (offset_t) 0;
        node->value.file.size = size;
    }

    else if (((size_t) offset + size) > node->value.file.size){
        for (file_block = (file_block_t*) offset_to_ptr(handle,
                    node->value.file.first_block);
                file_block->nxt_file_block != (offset_t) 0;
                file_block = (file_block_t *) offset_to_ptr(handle,
                    file_block->nxt_file_block));

        file_block->nxt_file_block = allocate_memory(handle, FILE_BLOCK_SIZE);
        
        if (file_block->nxt_file_block == (size_t) 0){
            *errnoptr = ENOMEM;
            return -1;
        }

        file_block = (file_block_t *) offset_to_ptr(handle, 
                file_block->nxt_file_block);

        file_block->data = allocate_memory(handle, size);
        
        if (file_block->data == (size_t) 0){
            *errnoptr = ENOMEM;
            return -1;
        }
        file_block->nxt_file_block = (offset_t) 0;

        file_block->block_size = size;
        node->value.file.size += size;
    }

    num_bytes = (int) size;
    memcpy(offset_to_ptr(handle, file_block->data),
            buf, size);

    return num_bytes;
}

/* Implements an emulation of the utimensat system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call changes the access and modification times of the file
   or directory indicated by path to the values in ts.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 utimensat.

*/
int __myfs_utimens_implem(void *fsptr, size_t fssize, int *errnoptr,
                          const char *path, const struct timespec ts[2]) {

    //printf("UTIMES %s\n", path);

    super_block_t *handle; 
    inode_t *node;

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;  
        return -1;
    }

    node = get_path(handle, path);
    if (node == NULL){
        *errnoptr = ENOENT;
        return -1;
    }

    node->acc_time = ts[0];
    node->mod_time = ts[1];
     
    return 0;
}

/* Implements an emulation of the statfs system call on the filesystem 
   of size fssize pointed to by fsptr.

   The call gets information of the filesystem usage and puts in 
   into stbuf.

   On success, 0 is returned.

   On failure, -1 is returned and *errnoptr is set appropriately.

   The error codes are documented in man 2 statfs.

   Essentially, only the following fields of struct statvfs need to be
   supported:

   f_bsize   fill with what you call a block (typically 1024 bytes)
   f_blocks  fill with the total number of blocks in the filesystem
   f_bfree   fill with the free number of blocks in the filesystem
   f_bavail  fill with same value as f_bfree
   f_namemax fill with your maximum file/directory name, if your
             filesystem has such a maximum

*/
int __myfs_statfs_implem(void *fsptr, size_t fssize, int *errnoptr,
                         struct statvfs* stbuf) {

    super_block_t *handle;
    size_t bsize;

    //printf("STATFS\n");

    bsize = (size_t) 1024;

    handle = get_handle(fsptr, fssize);
    if (handle == NULL){
        *errnoptr = EFAULT;
        return -1;
    }

    memset(stbuf, 0, sizeof(struct statvfs));
    stbuf->f_bsize = (__fsword_t) bsize;
    stbuf->f_blocks = (fsblkcnt_t) (handle->size / bsize);
    stbuf->f_bfree = (fsblkcnt_t) (free_size(handle) / bsize);
    stbuf->f_bavail= stbuf->f_bfree;
    stbuf->f_namemax = MAX_FILE_NAME;
  return 0;
}

