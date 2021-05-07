# !/bin/bash
gcc -o main main.c bt/lib/libbt.a `pkg-config --cflags gtk+-3.0` `pkg-config --libs gtk+-3.0` -rdynamic -g -Ibt/inc -w