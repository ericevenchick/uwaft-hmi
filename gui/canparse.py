import time
import os
def get_key(msg): 
	# split the message into parts
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
		return None

def get_soc(msg):
	data = msg.split('-')
	soc = int(data[3], 16)
	return soc

def get_hvil(msg):
	data = msg.split('-')
	hvil = int(data[2][-3], 16)
	return hvil

