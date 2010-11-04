import time
import os
def getpos(msg): 
	data = msg.split('-')
	key = int(data[2][-2:], 16)
	if key == 128:
		return "Off"
	elif key == 133:
		return "Acc"
	elif key == 174:
		return "On"
	elif key == 170:
		return "Crank"
	else:
		pass

if __name__ == "__main__":
	f = open("/home/eric/uwaft-hmi/canmsg", "r+")
	lastline = ""
	while 1:
		line = getpos(f.readline())
		if line != lastline:
			print line
			lastline = line

