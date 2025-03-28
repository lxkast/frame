# Frame
A proof-of-concept Windows kernel driver that spoofs the current thread and machine frame for NMI callbacks on x86-64.
# How it works
By hooking `HalPreprocessNmi` from the HAL private dispatch table, we can swap the current thread in the KPRCB for the core's idle thread, as well as swap the RIP and RSP from the machine frame to appropriate values for the idle thread.
To restore the original machine frame and current thread, we can manually traverse the NMI callback linked list and add our restoration function to the end.

A full writeup with detailed explanation will be published in the near future.
# Screenshots
![image](https://github.com/user-attachments/assets/9e05d615-8f4a-4933-8c44-346d58ebb136)
![image](https://github.com/user-attachments/assets/ea50c0a7-4608-4d8a-9524-48e2db7e08b1)
