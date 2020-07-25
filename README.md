# parallel.datacollector.vclab
Parallel data-collecting framework with multi-processing and multi-threading. The framework takes the IMI A200 camera as an example but also allows to add other camera modules provided by camera vendors. The data collector can open an arbitrary number of cameras simultaneously at a high frame rate. 

## Compile
1. Install [OpenGL](https://www.opengl.org/) and [freeglut](http://freeglut.sourceforge.net/).
2. Install [kwhat/libuiohook](https://github.com/kwhat/libuiohook) for global keyboard events. You can also choose other global keyboard libraries as well. Just write your own codes in `hotkeys()`.
2. Use `cmake` to compile, or type `./make.sh` in the commmand line.

## Use

First, connect the A200 camera/s to PC.

#### Run
```
./UVCDepthIRViewer [camera_id] 

The camera_id starts from 0. If camera_id is not given, the program will use all the cameras.
```

This command will collect three modalities (RGB, Depth, and IR) simutaneously.

#### Label Configuration
The program can save a frame with a specific `label` for classification tasks. The `config.txt` file defines all the labels. Simply add your label information to each line of `config.txt`. 

#### Keyboard Control
```
- '0-9': select the pre-defined labels
- 'Uparrow / Downarrow': switch the pre-defined labels
- 's': save 10 frames with label
- 'ESC': quit the program
```
