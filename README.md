# vclab.datacollector.a200
Collect RGB / Depth / IR images via IMI A200 camera

## Compile
1. Install [OpenGL](https://www.opengl.org/) and [freeglut](http://freeglut.sourceforge.net/).
2. Install [kwhat/libuiohook](https://github.com/kwhat/libuiohook) for global keyboard events. You can also choose other global keyboard libraries as well.
2. Use `cmake` to compile, or just type `./make.sh` in the commmand line.

## Use

First, connect the A200 camera/s to PC.

#### Run
```
./UVCDepthIRViewer [camera_id] 

The camera_id starts from 0. If camera_id is not given, the program will use all the cameras.
```

This command will collect three modalities (RGB, Depth, and IR) simutaneously.

#### Keyboard Control
```
- '0-9': select the pre-defined labels
- 'Uparrow / Downarrow': switch the pre-defined labels
- 's': save 10 frames with label
- 'ESC': quit the program
```
