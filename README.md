# Context
This is a little project inspired by an SI (science de l'ingenieur) lesson in High School back in 2018.
The following is a dump of this project.

# What is it?
This program can encode data in a .bmp image by modifying the least significant bits of pixels. Any type of file can be encoded inside as long as the size is smaller than a threshold.

# Usage

To encode data inside a .bmp image, first create a new config file with the `steg -new config.txt` command. Then fill all the information inside:
```
bitmap:example.bmp
data:example.example
output:example.bmp
bit-offset:0
encrypt:1
```
*Notice: using bit-offset will reduce maximum file size that can be encoded.*
Once you filled in the information, run `steg config.txt`.
You should now see a new file created by the program.

To decode data from an image, run `steg -decode <name>.bmp`. 
