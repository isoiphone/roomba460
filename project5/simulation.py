#!/usr/bin/env python
import pygame
from pygame.locals import *
import os,random,math

kScreenSize = (512,512)
kWindowCaption = "Roomba Simulation"
kFramesPerSecond = 20
kClrBlack = (0,0,0)
kClrBumper = (128,128,255)
kClrRoomba = (128,128,128)

kDriveSpeed = 300       # speed roomba drives at, will be dynamic in future, for now is a constant
kSomeRad = 1000         # some arbitrary rad for the patterns to be scaled based on

kMillimetersToPixels = 1.0 / 10.0 # 10mm per pixel, aka 100 pixel per meter
kRoombaRadius = 170. # 34 cm in diameter says wiki, so 34/2 radius.
kLedRadius = 20.0   # 2cm
kRoombaWheelSpacing = 258. # 258mm between wheels says the SCI spec document



class Roomba:
    "current position / heading"
    m_velocity = 0.
    m_radius = 100*1000*1000.
    
    "actual position in cm"
    m_position = [int((kScreenSize[0]/2.)/kMillimetersToPixels),int((kScreenSize[1]/2.)/kMillimetersToPixels)]
    m_heading = 0.
    
    "angle and distance traveled since last time sensors were read"
    m_angle = 0.
    m_distance = 0.
    
    "LED state R,G,B"
    m_led = (0,0,0)
    
    "offsceen surface used to draw persistent lines to"
    m_offscreen = None
    
    "manually place the roomba in some starting position facing some direction"
    def place(self, position, heading):
        self.m_position = position
        self.m_heading = heading
        
    """
    send drive command to roomba
    units are mm/s and mm, negative radius means clockwise
    """
    def drive(self, velocity, radius):
        self.m_velocity = float(velocity)
        
        if radius == 0:
            self.m_radius = 100*1000*1000. # 1000m radius is close enough to infinite for me
        else:
            self.m_radius = float(radius)
        """
        elif radius < 0 and radius > -50:
            self.m_radius = -50.0
        elif radius > 0 and radius < 50:
            self.m_radius = 50.0
        else:
            self.m_radius = float(radius)
        """
        
    def setLED(self, r,g,b):
        self.m_led = (r,g,b)
        
    """
    read sensors
    D = (dR + dL) / 2
    A = (dR - dL) / 2
    """
    def getSensors(self):
        sensors = (int(self.m_angle),int(self.m_distance))
        self.m_angle = self.m_distance = 0.
        return sensors
    
    def update(self,elapsedMs):
        #self.m_position[0] += (340. * elapsedMs) / 1000.0
        elapsedS = elapsedMs / 1000.0
        
        if self.m_radius == -1 or self.m_radius == +1:
            theta = elapsedS * math.pi
            self.m_heading += theta
            
            diff = theta*258.0*0.5
            self.m_angle += diff
            return
            
        # turning radius
        r = self.m_radius
        
        # distance we traveled along the arc
        s = self.m_velocity * elapsedS
        
        # angle it makes with the turning radius
        theta = s / r

        # straight-line distance traveled (chord length)
        c = 2.0*r*math.sin(theta/2.0)
        
        # update heading
        self.m_heading += theta
        
        dx = c * math.cos(self.m_heading)
        dy = c * math.sin(self.m_heading)
        
        # update position
        self.m_position[0] += dx
        self.m_position[1] += dy
        
        # update sensors
        right_wheel_dist = (r-(258.0/2.)) * theta
        left_wheel_dist = (r+(258.0/2.)) * theta
        
        self.m_angle += int(right_wheel_dist-left_wheel_dist)/2
        self.m_distance += c
        
    def draw(self,screen):
        if self.m_offscreen == None:
            self.m_offscreen = screen.copy()
            self.m_offscreen.fill(kClrBlack)
            self.m_offscreen.set_colorkey(kClrBlack)
            
        pos = int(round(self.m_position[0]*kMillimetersToPixels)), int(round(self.m_position[1]*kMillimetersToPixels))

        # body of roomba
        radius = int(round(kRoombaRadius*kMillimetersToPixels))
        pygame.draw.circle(screen, kClrRoomba, pos, radius)
        
        # the front bumper
        x1 = math.cos(self.m_heading-math.pi/4.0)*kRoombaRadius*kMillimetersToPixels
        x2 = math.cos(self.m_heading+math.pi/4.0)*kRoombaRadius*kMillimetersToPixels
        y1 = math.sin(self.m_heading-math.pi/4.0)*kRoombaRadius*kMillimetersToPixels
        y2 = math.sin(self.m_heading+math.pi/4.0)*kRoombaRadius*kMillimetersToPixels
        pygame.draw.line(screen, kClrBumper, (pos[0]+x1,pos[1]+y1), (pos[0]+x2,pos[1]+y2), 2)
        
        # the LED
        if self.m_led != (0,0,0):
            pygame.draw.circle(self.m_offscreen, self.m_led, pos, int(round(kLedRadius*kMillimetersToPixels)))
            
        screen.blit(self.m_offscreen, (0,0))
        
    

# a plan is a series of steps
# each step is a precondition, velocity, radius, and duration.
HALT = 0        # just stop, no args
ARC = 1         # draw an arc, arg1=radius of the full circle drawn, arg2=how much of the circle to draw (the angle of the pie slice of it) in degrees
SPIN = 2        # spin on the spot, arg1=angle to turn through (same as arg2 of arc) in degrees
LED = 3         # do something with the LED, arg1=command to send to LED (1 is on, 0 is off)

class Command:
    "structure that base station uses to represent an image / turtle command"
    def __init__(self, command, arg1=None, arg2=None, arg3=None):
        self.m_command = command
        self.m_arg1 = arg1
        self.m_arg2 = arg2
        self.m_arg3 = arg3

PARKED = 0
ARCING = 1
SPINNING = 2

class Turtle:
    "structure that base station uses to represent a roomba"
    # radio address for this roomba
    m_address = 0
        
    # sensor value accumulators
    m_angle = 0
    m_distance = 0

    # value we are trying to reach in this state (could use a distance or a time instead, for now we use angle)
    m_angleTarget = 0

    # strictly speaking, we dont need to keep track of state, it can be derived from
    # the current command being executed. However it is kept as a convenience
    m_state = PARKED

    m_plan = (
                #Command(LED,0x00,0x00,0x00), Command(ARC,kSomeRad,30), Command(SPIN,kSomeRad,30),
              #Command(LED,0xEC,0x58,0x00), Command(ARC,kSomeRad*0.5,360),   # inner fruit
              
              Command(LED,0x52,0x18,0xFA), Command(ARC,kSomeRad,120),   # 1/2 petal
              Command(LED,0x6B,0x3F,0xA0), Command(ARC,kSomeRad,120),   # outer space
              Command(LED,0x52,0x18,0xFA), Command(ARC,kSomeRad,120),   # 1/2 petal
              Command(SPIN,60), # offset
              Command(LED,0x52,0x18,0xFA), Command(ARC,kSomeRad,120),   # 1/2 petal
              Command(LED,0x6B,0x3F,0xA0), Command(ARC,kSomeRad,120),   # outer space
              Command(LED,0x52,0x18,0xFA), Command(ARC,kSomeRad,120),   # 1/2 petal
              Command(SPIN,60), # offset
              Command(LED,0x52,0x18,0xFA), Command(ARC,kSomeRad,120),   # 1/2 petal
              Command(LED,0x6B,0x3F,0xA0), Command(ARC,kSomeRad,120),   # outer space
              Command(LED,0x52,0x18,0xFA), Command(ARC,kSomeRad,120),   # 1/2 petal
              )

    m_index = -1


class Simulation:
    m_curTimeMs = 0.
    m_roombas = [Roomba(), Roomba()]
    m_turtles = [Turtle(), Turtle()]
    
    def issueNextCommand(self, index):
        #print "issueNextCommand()"
        t = self.m_turtles[index]
        r = self.m_roombas[index]
        
        # next step
        t.m_index += 1
        
        # if we hit the end of the plan, stop
        if t.m_index >= len(t.m_plan):
            t.m_state = PARKED
            r.setLED(0,0,0)
            r.drive(0,0)
            return
        
        # clear sensor accumulators
        t.m_angle = t.m_distance = 0
        
        # clear target
        t.m_angleTarget = 0
        
        # decode next command
        cmd = t.m_plan[t.m_index]
        if cmd.m_command == HALT:
            t.m_state = PARKED
            r.setLED(0,0,0)
            r.drive(0,0)
        elif cmd.m_command == ARC:
            t.m_state = ARCING
            t.m_angleTarget = cmd.m_arg2
            r.drive(kDriveSpeed, cmd.m_arg1)
        elif cmd.m_command == SPIN:
            t.m_state = SPINNING
            t.m_angleTarget = cmd.m_arg1
            # clockwise = -1, counterclockwise = 1
            if cmd.m_arg1 <= 0:
                r.drive(kDriveSpeed, 1)
            else:
                r.drive(kDriveSpeed, -1)
        elif cmd.m_command == LED:
            r.setLED(cmd.m_arg1,cmd.m_arg2,cmd.m_arg3)
            self.issueNextCommand(index)
        
    def executePlans(self):
        for i in range(len(self.m_turtles)):
            t = self.m_turtles[i]
            r = self.m_roombas[i]
            
            # accumulate sensor readings for each turtle
            s = r.getSensors()
            t.m_angle += s[0]
            t.m_distance += s[1]
            
            # todo: halt if we get a bumper sensor reading
            
            turnedThrough = ((360.0 * t.m_angle) / (258.0 * math.pi)) # in degrees, with clockwise meaning being negative

            # if we are parked and not done our plan, 
            if t.m_state == PARKED and t.m_index < len(t.m_plan):
                self.issueNextCommand(i)
            # if we have completed this arc, or if we have completed this spin, 
            elif t.m_state == ARCING or t.m_state == SPINNING:
                if abs(turnedThrough) >= abs(t.m_angleTarget):
                    self.issueNextCommand(i)
            
            #print i, ("parked", "arcing", "spinning")[t.m_state]

            
    def start(self):
        center = [int((kScreenSize[0]/2.)/kMillimetersToPixels),int((kScreenSize[1]/2.)/kMillimetersToPixels)]
        
        # start back-to-back in the center
        self.m_roombas[0].place([center[0]-kRoombaRadius, center[1]], math.pi)
        self.m_roombas[1].place([center[0]+kRoombaRadius, center[1]], 0)
        
    def update(self, elapsedMs):
        # simulation
        self.m_curTimeMs += elapsedMs
        for r in self.m_roombas:
            r.update(elapsedMs)
        
        # the commanding unit
        self.executePlans()
    
    def draw(self, screen):
        for r in self.m_roombas:
            r.draw(screen)
    
def main():
    random.seed()
    pygame.init()
    pygame.display.set_caption(kWindowCaption)
    screen = pygame.display.set_mode(kScreenSize)
    clock = pygame.time.Clock()
    clock.tick(kFramesPerSecond)
    
    screen.fill(kClrBlack)
    pygame.display.update()

    simulation = Simulation()
    simulation.start()
    
    while 1:
        event = pygame.event.poll()
        if event.type == QUIT:
            break
        
        simulation.update(clock.get_time())
        
        screen.fill(kClrBlack)
        simulation.draw(screen)
        
        pygame.display.update()
        clock.tick(kFramesPerSecond)
        
if __name__ == "__main__":
    try:
        main()
    finally:
        pygame.quit()
