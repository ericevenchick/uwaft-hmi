import time
import os
def get_key(msg): 
	# split the message into parts
	data = msg.split('-')
	# GMLAN ID containing key position data
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
		return None

if __name__ == "__main__":
	f = open("/home/eric/uwaft-hmi/canmsg", "r+")
	lastline = ""
	while 1:
		line = getpos(f.readline())
		if line != lastline:
			print line
			lastline = line

