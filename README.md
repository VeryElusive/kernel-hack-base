Test implementation of a kernel game hack.

Comes with the driver project and driver manual map.

Plenty of spaghetti code here- I wrote this purely as a test implementation.

Communication between usermode and kernel is done through a buffer allocated in the usermode process (no callbacks, and thus less detection vector)

Usermode process loads the kernel driver through the following method:
 - Run a vulnerable intel driver which has access to allocating/writing to kernel through IOCTL control, and the ability to call function.
 - Get the vulnerable driver to allocate the buffer in kernel space.
 - Manually map the driver (solve relocations, fix imports etc.) and then write it into memory.
 - Create a JMP to the driver's entry function by modifying the memory of a kernel function (NtAddAtom since it has a call instruction near the beginning)
 - Unload the vulnerable driver and remove all traces of it to prevent detection

We then draw by creating a transparent window which overlays on top of the game and draw using D2D.
 
Kernel driver reads/writes physical memory. Way stealthier.
