#ifndef JOYSTICK_TO_MOVEMENT_H_
#define JOYSTICK_TO_MOVEMENT_H_

// map X and Y values from a joystick to roomba velocity and radius.
// uses some lookup tables, and depends on previous calls to do some smoothing
void joystick_to_movement(uint8_t x, uint8_t y, int16_t* velocity, int16_t* radius);
    
#endif