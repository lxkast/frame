# Frame
A proof-of-concept Windows kernel driver that spoofs the current thread, machine frame and consequently the call stack for NMI callbacks on x86-64.
# How it works
By hooking `HalPreprocessNmi` from the HAL private dispatch table, we can swap the current thread in the KPRCB for the core's idle thread, as well as swap the RIP and RSP from the machine frame to appropriate values for the idle thread.
To restore the original machine frame and current thread, we can manually traverse the NMI callback linked list and add our restoration function to the end.

A full writeup with detailed explanation can be found here: 
https://lxkast.github.io/posts/Thread-and-call-stack-spoofing-for-NMI-callbacks-on-Windows/
# Screenshots
![image](https://github.com/user-attachments/assets/9e05d615-8f4a-4933-8c44-346d58ebb136)
![image](https://github.com/user-attachments/assets/ddea252f-4d15-4a5f-ad38-53db8a70ff6f)
