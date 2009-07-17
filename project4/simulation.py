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

kMillimetersToPixels = 1.0 / 10.0 # 10mm per pixel, aka 100 pixel per meter
kRoombaRadius = 170. # 34 cm in diameter says wiki, so 34/2 radius.
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
        elif radius < 0 and radius > -50:
            self.m_radius = -50.0
        elif radius > 0 and radius < 50:
            self.m_radius = 50.0
        else:
            self.m_radius = float(radius)
    
    """
    read sensors
    D = (dR + dL) / 2
    A = (dR - dL) / 2
    """
    def getSensors(self):
        sensors = (self.m_angle,self.m_distance)
        self.m_angle = self.m_distance = 0
        return tmp
    
    def update(self,elapsedMs):
        #self.m_position[0] += (340. * elapsedMs) / 1000.0
        elapsedS = elapsedMs / 1000.0
        
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
        self.m_angle += theta
        self.m_distance += c
        
    def draw(self,screen):
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
    
    
class Simulation:
    m_roomba1 = Roomba()
    m_roomba2 = Roomba()
    
    # a plan is a series of steps
    # each step is a precondition, velocity, radius, and duration.
    
    #m_plan = []
    m_curTimeMs = 0.
    m_curState = 0
    
    def start(self):
        center = [int((kScreenSize[0]/2.)/kMillimetersToPixels),int((kScreenSize[1]/2.)/kMillimetersToPixels)]
        
        # start back-to-back in the center
        self.m_roomba1.place([center[0]-kRoombaRadius, center[1]], math.pi)
        self.m_roomba2.place([center[0]+kRoombaRadius, center[1]], 0)
        
        # just one spiraling
        self.m_roomba1.drive(kRoombaRadius*8.0, 500)
        self.m_curState = 1
    
    def update(self, elapsedMs):
        self.m_curTimeMs += elapsedMs
        
        # after 1 second of just one spiraling, get the other spiraling too
        if self.m_curState == 1 and self.m_curTimeMs > 1000:
            self.m_roomba2.drive(kRoombaRadius*8.0, 500)
            self.m_curState = 2
            
        self.m_roomba1.update(elapsedMs)
        self.m_roomba2.update(elapsedMs)
    
    def draw(self, screen):
        self.m_roomba1.draw(screen)
        self.m_roomba2.draw(screen)
    
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
