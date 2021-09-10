# What is it?
This program can encode data in a .bmp image by modifying the least significant bits of pixels. Any type of file can be encoded inside as long as the size is smaller than a threshold.

# Usage

To encode data inside a .bmp image, first create a new configuration file with the `steg -new config.txt` command. Then fill all the information inside:
```
bitmap:example.bmp
data:example.example
output:example.bmp
bit-offset:0
encrypt:1
password_char:0
```
- "bitmap" is the target image in which data will be encoded.
- "data" is the data to be encoded.
- "output" is the name of the file which will consist of the "bitmap" file modified with the data inside.
- "bit-offset" spreads the data inside the target by skipping bits.
- "encrypt" should the data be encrypted or not? 1 - yes 0 - no
- "password_char" single character key to xor the data with.

*Note: 
using "bit-offset" will reduce maximum file size that can be encoded. 
"password_char" is one char length, which means it should be one character! leave it at 0 if you don't want password encryption...*

Once you filled in the information, run `steg config.txt`.
You should now see a new file created by the program.

To decode data from an image, run `steg -decode <name>.bmp`. 
