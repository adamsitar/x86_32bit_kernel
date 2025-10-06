# x86_32bit_kernel

A hobby 32-bit x86 kernel developed in C and NASM assembly, with the goal of exploring low-level system programming, virtual memory/segmentation, interrupt handling and general OS functionalities

## Feature examples: 

### Post initialization screen:

<img width="500" height="264" alt="os_post_init_screen" src="https://github.com/user-attachments/assets/f5ce37cb-a192-4285-a25b-7c9ee76bd2da" />

### Basic allocation:

<img width="668" height="270" alt="os_example_allocation" src="https://github.com/user-attachments/assets/5d4c6eed-0a17-4266-8775-23b10bc9769d" />

## Roadmap:

- transition entire project from Makefile + C to C++ and Cmake
- replace 32 bit GRUB with 64 bit Limine
- transition to multiprocessor architecture/approach  

## Build instructions:

Run the default `make` target
