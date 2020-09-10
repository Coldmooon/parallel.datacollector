# parallel.datacollector
Parallel data-collecting framework with multiprocessing and multithreading. The framework takes the IMI A200 camera as an example but also allows to add other camera modules provided by camera vendors. The data collector can open an arbitrary number of cameras simultaneously at a high frame rate. 

## Compile
1. Install [OpenGL](https://www.opengl.org/), [freeglut](http://freeglut.sourceforge.net/), and [OpenCV3](https://opencv.org/).
2. Install [kwhat/libuiohook](https://github.com/kwhat/libuiohook) for global keyboard events. You can also choose other global keyboard libraries as well. Just write your own codes in the `hotkeys()` function.
3. type `cmake .`, then type `make` to compile.

## Use

First, connect cameras to PC, e.g., the A200 camera.

#### Run
```
./UVCDepthIRViewer [camera_id] 

The camera_id starts from 0. If camera_id is not given, the program will use all the cameras.

./T3S 

T3S can capture temperature information.

```

The two commands can run concurrently with multi instances and collect four modalities (RGB, Depth, IR, and Temperature) simutaneously.

#### Label Configuration
The program can save a frame with a specific `label` for classification tasks. The `config.txt` file defines all the labels. Simply add your label information to each line of `config.txt`. 

#### Keyboard Control
```
- 'Uparrow / Downarrow': switch the pre-defined labels
- 'Leftarrow / Rightarrow': decrease / increase the perspective label
- '0-9,A-z': record a person name.
- 'F1': save 30 frames with a specific label
- 'F5': archive photos of a specific person
- 'Backspace': delete a characher from the person name.
- 'TAB': clean the person name.
- 'ESC': quit the program
```

#### Pipeline

```
0) connect a camera to a USB port.
1) start the programs by "./UVCDepthIRViewer 0" and "./T3S".
2) Input the person name.
3) adjust the perspective label and the task label.
4) press 'F1' to save 30 frames with pre-defined labels.
5) press 'F5' to archive the person's photos.
```


## Issues
```
error while loading shared libraries: libxxxx.so: cannot open shared object file: No such file or directory

PS: The libxxxx.so is a camera's library.
```
This error could appear at the running stage. This is because the `dynamic loader` couldn't find the `libxxxx.so` in some standard paths, `e.g., /usr/local/lib`. `Library Search Path` only tells `dynamic linker` where to get library to link executable to. But it doesn't tell `dynamic loader` where to search for the library when the executable is started. If the path to `libxxxx.so` is NOT absolute, `dynamic loader` tries to load it from some standard paths. You can run `ldd the_executable` to find out where `dynamic loader` will look for `libxxxx.so`. To solve this, put `libxxxx.so` in the standard paths, or run the executable with `LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/libxxxx.so`, or compile the executable with the absolute path to `libxxxx.so`. 

