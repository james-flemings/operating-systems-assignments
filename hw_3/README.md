# FUSE File-System

In this assignment, I was tasked to implement a file-system using [FUSE](https://github.com/libfuse/libfuse). The source file `myfs.c` was given to me by my instructor. Commands to properly run everything is given below: 

```bash
# You can create the file system by running the following commands: 
gcc -g -O0 -Wall myfs.c implementation.c `pkg-config fuse --cflags --libs` -o myfs

# The filesystem can be mounted while it is running inside gdb (for
# debugging) purposes as follows (adapt to your setup):

gdb --args ./myfs --backupfile=test.myfs ~/fuse-mnt/ -f

# It can then be unmounted (in another terminal) with

fusermount -u ~/fuse-mnt
```

The filesystem supports the following operations:

1. create files: `touch`
2. create directories: `mkdir`
3. create files filled with zeros: `truncatei`
4. show statistics of file: `stat`
5. read and write files: `echo "..." >`; `cat`
6. remove files and directories: `rm`; `rm -r`
7. rename files: `mv`

More information about the architecture of the system and known bugs can be found in `write_up.pdf`
