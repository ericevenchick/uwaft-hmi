import time
import os
def get_key(msg): 
	# split the message into parts
	data = msg.split('-')
	# mask the key bits (byte 1, bits 1-2)
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
	# get SOC byte (byte 2)
	soc = int(data[3], 16) & 0xFF
	# scale
	soc = soc/2
	return soc

def get_hvil(msg):
	data = msg.split('-')
	# mask the HVIL bit (byte 1,bit 3)
	hvil = int(data[2], 16) & 0x4
	if hvil > 0:
		return "On"
	else:
		return "Off"
def get_h2_alarm(msg):
	data = msg.split('-')
	# get H2 alarm bit (byte 1, bit 4)
	status = int(data[2], 16) & 0x8
	if status > 0:
		return True
	else:
		return False

def get_temp(msg):
	data = msg.split('-')
	# get temp byte (byte 3)
	status = int(data[4], 16) & 0xFF
	return status
