#!/usr/bin/env python
import pygame
from pygame.locals import *
import os,random

kScreenSize = (512,512)
kWindowCaption = "Roomba Simulation"
kFramesPerSecond = 20
kClrBlack = (0,0,0)
kClrBumper = (128,128,255)
kClrRoomba = (128,128,128)
kMetersToPixels = 100.0 # 1cm = 1pixel -> 1m = 100pixels
kRoombaRadius = 0.17 # 34 cm in diameter says wiki
kRoombaWheelSpacing = 0.258 # 258mm between wheels says the SCI spec document

class Roomba:
    "current position / heading"
    m_velocity = 0
    m_radius = 0
    m_position = [(kScreenSize[0]/2.)/kMetersToPixels,(kScreenSize[1]/2.)/kMetersToPixels]
    
    "angle and distance traveled since last time sensors were read"
    m_angle = 0
    m_distance = 0
    
    """
    send drive command to roomba
    units are mm/s and mm, negative radius means clockwise
    """
    def drive(self, velocity, radius):
        m_velocity = velocity
        m_radius = radius
    
    """
    read sensors
    D = (dR + dL) / 2
    A = (dR - dL) / 2
    """
    def getSensors(self):
        sensors = (m_angle,m_distance)
        m_angle = m_distance = 0
        return tmp
    
    def update(self,elapsedMs):
        pass
    
    def draw(self,screen):
        pos = int(round(self.m_position[0]*kMetersToPixels)), int(round(self.m_position[0]*kMetersToPixels))
        radius = int(round(kRoombaRadius*kMetersToPixels))
        rect = (pos[0]-radius, pos[1]-radius, radius*2, radius)
        
        # body of roomba
        pygame.draw.circle(screen, kClrRoomba, pos, radius)
        # the front bumper
        pygame.draw.ellipse(screen, kClrBumper, rect)

def main():
    random.seed()
    pygame.init()
    pygame.display.set_caption(kWindowCaption)
    screen = pygame.display.set_mode(kScreenSize)
    clock = pygame.time.Clock()
    
    screen.fill(kClrBlack)
    pygame.display.update()

    roomba = Roomba()
    roomba.drive(500, 0)
    
    while 1:
        event = pygame.event.poll()
        if event.type == QUIT:
            break
        
        roomba.update(clock.get_rawtime())
        roomba.draw(screen)
        
        pygame.display.update()
        pygame.display.flip()
        clock.tick(kFramesPerSecond)
        
if __name__ == "__main__":
    try:
        main()
    finally:
        pygame.quit()
