## Note

This is first code i am posting here. It is not meant to be useful code, more like excercise to learn new stuff about hardware.
In this excercise i am trying to drive HUB75 panels with bitbanging pins.

# Setup

To setup code we need to assign pins accordingly:
```
#define R0 7
#define G0 8
#define B0 23

#define R1 21
#define G1 22
#define B1 20

#define A 6
#define B 5
#define C 4
#define D 3

#define CLK 24
#define LAT 32
#define OE  25
```
Then we can select number of panels per column and row and single panel size:
```
#define ROW_SIZE 64
#define COL_SIZE 32

#define PANELS_W 1
#define PANELS_H 1
```
