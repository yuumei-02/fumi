# About
Fumi is a programming language made for recreational and learning purposes.

# Building the project
This project depends on Mektova-C-Utils,          : https://github.com/yuumei-02/Mektova-C-Utils <br>
And Vlodinnye-make (already included in the repo) : https://github.com/yuumei-02/Vlodinnye-make <br>
Mektova-C-Utils needs to be installed system-wide. <br>
<br>
To build the project, bootstrap the build system by running the following command inside the project root folder.
```shell
gcc -Wall -Wextra -pedantic -std=c23 vmake.c -o vmake -lmcu-debug
```
Then, just run ```./vmake``` and your done.

# Roadmap
[ ] Asm generation for x86-64 Linux<br>
[ ] Turing complete<br>
[ ] Statically and strongly typed<br>
[ ] Self hosted<br>
[ ] Own assembler<br>

