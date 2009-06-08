/** @author Scott Craig
 *
 *  @brief Stepper motor sample application.
 */
#ifndef __STEPPER_H__
#define __STEPPER_H__


// Choose 1 for half-stepping, 2 for full-stepping
//#define STEP    1
#define STEP 2

#define STEPPER_RED     _BV(3)
#define STEPPER_BLUE    _BV(2)
#define STEPPER_WHITE   _BV(1)
#define STEPPER_YELLOW  _BV(0)
  

#define STEPPER_PORT        PORTC
#define STEPPER_PORT_DDR    DDRC
#define STEPPER_MASK    (STEPPER_RED | STEPPER_BLUE | STEPPER_WHITE | STEPPER_YELLOW)

#endif
