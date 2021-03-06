# Memory Management 

In this assignment, I implemented functions that behave like malloc, calloc, realloc, and free using mmap and munmap system calls. To compile the files, use the following: 

```bash
gcc -fPIC -Wall -g -O0 -c memory.c 
gcc -fPIC -Wall -g -O0 -c implementation.c
gcc -fPIC -shared -o memory.so memory.o implementation.o -lpthread
```

Use the following commands to test it:

```bash
export LD_LIBRARY_PATH=`pwd`:"$LD_LIBRARY_PATH"
export LD_PRELOAD=`pwd`/memory.so 
export MEMORY_DEBUG=yes
ls
```

Make sure to exit the terminal after running those commands as the compiler will continue running with my implementation. As of right now, there's a known bus error bug if you run certain commands like `ls` on a ARM architecture. `write_up.pdf` provides more information about the program. 
