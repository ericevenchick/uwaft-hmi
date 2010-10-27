import time
import os
f = open("/home/eric/uwaft-hmi/canmsg", "r+")
lastkey = 0;
while 1:
    
    line = f.readline()
    msg = line.split('-')
    key = int(msg[2][-2:], 16)
    if key == lastkey:
       continue 
    if key == 128:
        print "Off"
    elif key == 133:
        print "Acc"
    elif key == 174:
        print "On"
    elif key == 170:
        print "Crank"
    lastkey = key
