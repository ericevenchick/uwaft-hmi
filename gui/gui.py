import clutter
import time
import os
import glib
import canparse
import bar
xsize = 500
ysize = 500
class gui:

	# handle data from the canbus
	def update(self, source, cond):
		# get new data from canbus
		# and remove the newline
		data = source.readline()
		data = data.strip()
		print data
		# get the message id so we can determine
		# what should be updated
		canid = -1
		try:
			canid = int(data.split('-')[0])
		except ValueError:
			pass
		try:
			# HMI_Tx1 Message
			if canid == 160:
				# display key data
				msg = canparse.get_key(data)		
				if msg != None: 
					self.keypos_label.set_text(msg)
				# display SOC data
				soc = canparse.get_soc(data)
				if soc == 100:
					self.soc_bar.set_segs(10)
				else:
					self.soc_bar.set_segs((soc/10)%11+1)
				self.soc_label.set_text("SOC: " + str(soc) + "%")
				# display HVIL on/off
				hvil = canparse.get_hvil(data)
				self.hvil_label.set_text("HVIL: " + str(hvil))
		except (IndexError):
			pass
		# return true to keep handling
		# input from the canbus pipe
		return True
	def close(self):
		canbus.close()
		clutter.main_quit()
	def __init__(self, canbusfile):
		
		# Open the can bus pipe
		self.canbus = open(canbusfile, "r+")
		
		# initialize the stage
		self.stage = clutter.Stage()
		self.stage.set_size(xsize,ysize)
		self.stage.set_color(clutter.Color(0,0,0))
		
		# Key Position 
		self.keypos_label = clutter.Text()
		self.keypos_label.set_text("Off")
		self.keypos_label.set_size(25,100)
		self.keypos_label.set_font_name("Helvetica 25")
		self.keypos_label.set_color(clutter.Color(255,255,255))
		self.keypos_label.set_position(xsize - 100, 5)
		self.stage.add(self.keypos_label)
		
		# SOC Display
		self.soc_label = clutter.Text()
		self.soc_label.set_text("SOC:")
		self.soc_label.set_size(100,25)
		self.soc_label.set_font_name("Helvetica 25")
		self.soc_label.set_color(clutter.Color(255,255,255))
		self.soc_label.set_position(40, 5)
		self.soc_bar = bar.bar(self.stage, 40, 40)
		self.stage.add(self.soc_label)
		
		# HVIL Enabled Display
		self.hvil_label = clutter.Text()
		self.hvil_label.set_text("")
		self.hvil_label.set_size(100,25)
		self.hvil_label.set_font_name("Helvetica 25")
		self.hvil_label.set_color(clutter.Color(255,255,255))
		self.hvil_label.set_position(40, 100)
		self.stage.add(self.hvil_label)	
		
		# let us quit
		self.stage.connect("destroy", clutter.main_quit)
		# set up to watch the canbus with handler
		# function self.update
		glib.io_add_watch(self.canbus, glib.IO_IN, self.update)
	
	def start(self):
		self.stage.show_all()
		clutter.main()

if __name__ == "__main__":
	g = gui()
	try:
		g.start()
	except (KeyboardInterrupt):
		pass
