# Buffer Hub Overview

## Design

We divide memory into the following four major levels:

+ Byte level
  Byte number ranges from 0 to 1023
+ KB level
  Byte number ranges from 1024 to 1024*1023
+ MB level
  Byte number ranges from 1024*1024 to 1024*1024*1023 
+ GB level
  Byte number ranges from 1024*1024*1024 to min(1024*1024*1024*1023,Device memory)

On top of that, we continue  divide into sub levels from major levels.

In byte level, we form the following sub levels
+ 16 bytes
+ 64 bytes
+ 256 bytes
  
In KB level, we form the following sub levels
+ 1 kb
+ 2 kb
+ 4 kb
+ 8 kb
+ 16 kb
+ 32 kb
+ 64 kb
+ 128 kb
+ 256 kb
+ 512 kb
  
In MB level, we form the following sub levels
+ 1 mb
+ 2 mb
+ 4 mb
+ 8 mb
+ 16 mb
+ 32 mb
+ 64 mb
+ 128 mb
+ 256 mb
+ 512 mb

In GB level, we form the following sub levels
+ 1 GB
+ 2 GB
+ 4 GB
+ 8 GB
+ 16 GB
+ 32 GB
+ 64 GB
+ 128 GB
+ 256 GB
+ 512 GB



## Usage
