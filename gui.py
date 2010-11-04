import clutter
import time
import os
import glib
import keypos
canbus = open("/home/eric/uwaft-hmi/canmsg", "r+")

class gui:

	# handle data from the canbus
	def update(self, source, cond):
		
		# get new data from canbus
		# and remove the newline
		data = source.readline()
		data = data.strip()
		
		# display the key position
		msg = str(keypos.getpos(data))		
		if msg != None: 
			self.text1.set_text(msg)
	 		
		# return true to keep handling
		# input from the canbus pipe
		return True
	def close(self):
		canbus.close()
		clutter.main_quit()
	def __init__(self):

		# initialize the stage
		self.stage = clutter.Stage()
		self.stage.set_size(500,500)
		self.stage.set_color(clutter.Color(0,0,0))
		
		# set up some text boxes
		self.text1 = clutter.Text()
		self.text1.set_text("Init")
		self.text1.set_size(400,400)
		self.text1.set_font_name("Helvetica 50")
		self.text1.set_color(clutter.Color(255,255,255))
		self.text1.set_position(200,200)
		self.stage.add(self.text1)
		
		# let us quit
		self.stage.connect("destroy", clutter.main_quit)
		# set up to watch the canbus with handler
		# function self.update
		glib.io_add_watch(canbus, glib.IO_IN, self.update)
	
	def start(self):
		self.stage.show_all()
		clutter.main()

if __name__ == "__main__":
	g = gui()
	try:
		g.start()
	except (KeyboardInterrupt):
		pass
