import time
import os

def get_bits(msg, startbit, length):
	data = msg.split('-')
	# byte to start on, add 2 to ignore id and DLC
	byte = startbit / 8 + 2
	# the bit number local to byte 
	bit = startbit % 8
	# the bit number, global to message
	bitnum = startbit
	value = 0
	for x in range(0, length):
		# is the xth byte a 1?
		if int(data[byte], 16) & (1 << bit):
			value = value + (1 << x)
		# if we will go to the next byte, go back one byte, start at bit 0
		if (bitnum + 1)/8 > bitnum/8:
			bit = 0
			byte = byte - 1
			bitnum = bitnum - 7
		else:
			bit = bit + 1
			bitnum = bitnum + 1

	return value

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
	if int(msg.strip().split('-')[0]) != 497:
		return
	key = get_bits(msg, 0, 2)
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
	#soc = get_value(msg, 2, 0xFF) 
	soc = get_bits(msg, 8, 8)
	# scale
	soc = soc/2
	return str(soc) + "%"

def get_hvil(msg):
	return get_value(msg, 1, 0x4)

def get_fcs(msg):
	return get_value(msg, 1, 0x10)

def get_h2_alarm(msg):
	status = get_value(msg, 1, 0x8)
	if status > 0:
		return True
	else:
		return False

def get_temp(msg):
	status = get_value(msg, 3, 0xFF)
	return str(status) + u"\u00b0" + "C"
