from pylab import scatter,show,title,xlabel,ylabel

# assume any joystick val with deflection less than this is 'centered'
DEADZONE = 10

# if rotation joystick is this close to edge then assume an 'on the spot' turn.
EDGEZONE = 5

# max turning radius (roomba allows -2000->+2000, but that sucks.)
MAX_RADIUS = 500

# max roomba speed (roomba allows -500->+500 but that also sucks.)
MAX_SPEED = 200

def smoothstep(v):
    return v*v*(3-2*v)

def velocity(x):
    if x > (128-DEADZONE) and x < (128+DEADZONE):
        return 0
    return round(smoothstep((255-x)/255.)*(2.*MAX_SPEED)-MAX_SPEED)

def radius(x):
    if x > (128-DEADZONE) and x < (128+DEADZONE):
        #return 0
        return 32768
    if x < EDGEZONE:
        return 1
    if x > 255-EDGEZONE:
        return -1
    if x < 128:
        r = round(smoothstep(x/127.)*MAX_RADIUS)
    if x > 128:
        x = 128 - (x - 128)
        r = round(smoothstep(x/127.)*-MAX_RADIUS)
    return int(r)
        
x = range(256)
vel = [velocity(item) for item in x]
rad = [radius(item) for item in x]

print "// map joystick values to TURN RADIUS"
print "// generated from python script"
print "const int16_t radius[256] = { "
for i in range(256):
    if i > 0 and i % 16 == 0:
        print ""
    print "%+5d," % rad[i], 
print "};"

print "\r\n\r\n"

print "// map joystick values to VELOCITY"
print "// generated from python script"
print "const int16_t velocity[256] = { "
for i in range(256):
    if i > 0 and i % 16 == 0:
        print ""
    print "%+3d," % vel[i], 
print "};"


scatter(x, vel, 1)
title('Velocity vs Y1')
xlabel('Joystick Y')
ylabel('Velocity')
show()


#scatter(x, rad, 1)
#title('Turn Radius vs X2')
#xlabel('Joystick X')
#ylabel('Turn Radius')
#show()
