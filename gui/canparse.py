import time
import os
def get_value(msg, byte, mask):
	# field 0 is ID, field 1 is DLC, bytes start at 2
	byte = byte + 1
	# split the message into parts
	data = msg.split('-')
	# mask the key bits)
	value = int(data[byte], 16) & mask
	return value

# take no action
def none(msg):
	return ""

def get_key(msg): 
	key = get_value(msg, 1, 0x3)
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
	soc = get_value(msg, 2, 0xFF) 
	# scale
	soc = soc/2
	return str(soc) + "%"

def get_hvil(msg):
	hvil = get_value(msg, 1, 0x4)
	if hvil > 0:
		return "On"
	else:
		return "Off"

def get_h2_alarm(msg):
	status = get_value(msg, 1, 0x8)
	if status > 0:
		return True
	else:
		return False

def get_temp(msg):
	status = get_value(msg, 3, 0xFF)
	return str(status) + u"\u00b0" + "C"
