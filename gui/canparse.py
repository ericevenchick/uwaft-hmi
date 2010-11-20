import time
import os
def get_key(msg): 
	# split the message into parts
	data = msg.split('-')
	# mask the key bits (bits 1-2)
	key = int(data[2], 16) & 0x3
	if key == 0:
		return "Off"
	elif key == 1:
		return "Acc"
	elif key == 2:
		return "On"
	elif key == 3:
		return "Crank"
	else:
		return None

def get_soc(msg):
	data = msg.split('-')
	# get SOC byte
	soc = int(data[3], 16) & 0xFF
	# scale
	soc = soc/2
	return soc

def get_hvil(msg):
	data = msg.split('-')
	# mask the HVIL bit (bit 3)
	hvil = int(data[2], 16) & 0x4
	if hvil > 0:
		return "On"
	else:
		return "Off"

