# vclab.datacollector.a200
Collect RGB / Depth / IR images via IMI A200 camera

## Compile
Use `cmake` to compile, or just type `./make.sh` in the commmand line.

## Use
```
./UVCDepthIRViewer [camera_id]
```

This will collect three modalities (RGB, Depth, and IR) simutaneously.

#### Keyboard Control
```
- '0-9': select the pre-defined labels
- 'Uparrow / Downarrow': switch the pre-defined labels
- 's': save 10 frames with label
- 'q': quit the program
```
