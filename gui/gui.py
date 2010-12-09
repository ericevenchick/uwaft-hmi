import clutter
import candisplay
import time
import os
import glib
import pages
import canparse
xsize = 750
ysize = 500

next_button_tex = clutter.cogl.texture_new_from_file('/home/eric/uwaft-hmi/gui/img/next-button.svg', clutter.cogl.TEXTURE_NO_SLICING, clutter.cogl.PIXEL_FORMAT_ANY)

class gui:
	# handle data from the canbus
	def update(self, source, cond):
		# get new data from canbus
		# and remove the newline
		data = source.readline()
		data = data.strip()
		
		# get the message id so we can determine
		# what should be updated
		canid = -1
		try:
			canid = int(data.split('-')[0])
		except ValueError:
			pass

		# update display elements
		for page in self.pages:
			for el in page:
				if hasattr(el, "can_update"):
					el.can_update(data)

		# display H2 Alarm
		#h2_alarm = canparse.get_h2_alarm(data)
		#if h2_alarm:
		#	self.h2_alarm_label.set_text("H2!")
		#	self.stage.set_color(clutter.Color(255,0,0))
		#else:
		#	self.h2_alarm_label.set_text("")
		#	self.stage.set_color(clutter.Color(0,0,0))

	def close(self):
		canbus.close()
		clutter.main_quit()
	def __init__(self, configfile):
		self.configfile = configfile

		# Setup info pages
		self.build_display(configfile)

		# Open the can bus pipe
		# self.canbus = open(canbusfile, "r+")
		
		# initialize the stage
		self.stage = clutter.Stage()
		self.stage.set_size(xsize,ysize)
		self.stage.set_color(clutter.Color(0,0,0))
	    
		# Next page button
		self.next_button = clutter.Texture()
		self.next_button.set_size(65, 50)
		self.next_button.set_position(xsize-70,0)
		self.next_button.set_cogl_texture(next_button_tex)
		self.next_button.set_reactive(True)
		self.next_button.connect("button-press-event", self.next_button_clicked)
		self.stage.add(self.next_button)
		

		# H2 Alarm Display (Page 0,1)
		self.h2_alarm_label = clutter.Text()
		self.h2_alarm_label.set_text("")
		self.h2_alarm_label.set_size(100,25)
		self.h2_alarm_label.set_font_name("Helvetica Bold 75")
		self.h2_alarm_label.set_color(clutter.Color(0,0,0))
		self.h2_alarm_label.set_position(xsize/2-75, ysize-100)
		self.stage.add(self.h2_alarm_label)
		# Start with page 0
		self.set_page(0)
		# allow quit
		self.stage.connect("destroy", clutter.main_quit)
		# reload when r is pressed
		self.stage.connect("key-press-event", self.key_pressed)


	# creates all pages based on configuration file
	def build_display(self,config):
		config_file = open(config, "r+")

		# get the number of pages
		self.numpages = 0
		for line in config_file.readlines():
			# ignore comments and blank lines
			if line[0] == "#" or line[0] == "\n":
				continue
			# check this line's page number
			newnumber = int(line.split(',')[0])
			# if it's the greatest so far, make that the number of pages
			if newnumber > self.numpages:
				self.numpages = newnumber
		# set the position back to the top of file
		config_file.seek(0)

		# The current page number (-1 for none)
		self.curpage = -1
		# store the pages and number of elements on each page
		self.pages = []
		elcount = []
		#populate the pages and page counts
		for i in range(0,self.numpages):
			self.pages.append([])
			# each count is [columns, rows]
			elcount.append([0,0])
		
		# iterate through the config and create the elements
		for line in config_file.readlines():
			# ignore comments and blank lines
			if line[0] == "#" or line[0] == "\n":
				continue
			page,disptype,args = line.strip().split(',',2)	
			# pages are 1 indexed, convert 
			page = int(page)-1
			
			# entire page display
			if disptype == 'p':
				if hasattr(pages, args):
					# call the builder function and pass the page to build
					getattr(pages, args)(self.pages[page])
				
			# boolean display
			if disptype == 'b':
				# create a constant label (no handler)
				newlabel = candisplay.can_label(args)
				# find the position for element
				y = 5+55*elcount[page][1]
				if y > ysize-50:
					# go to next column
					y = 5
					elcount[page][0] = elcount[page][0]+1
					elcount[page][1] = 0
				# each column has width 320
				x = 40 + elcount[page][0]*320
				newlabel.set_position(x,y) 
				self.pages[page].append(newlabel)

				# create the boolean display
				newbool = candisplay.can_bool(args)
				# just position relative to the label
				newbool.set_position(x + 220, y+7)
				self.pages[page].append(newbool)
				
				elcount[page][1] = elcount[page][1] + 1

			# text display
			if disptype == 't':
				newlabel = candisplay.can_label(args)
				y = 5+55*elcount[page][1]
				if y > ysize-50:
					y = 5
					elcount[page][0] = elcount[page][0]+1
					elcount[page][1] = 0
				x = 40 + elcount[page][0]*320
				newlabel.set_position(x,y) 
				self.pages[page].append(newlabel)
				elcount[page][1] = elcount[page][1] + 1
			
			# number display
			if disptype == 'n':
				newlabel = candisplay.can_number(args)
				y = 5+55*elcount[page][1]
				if y > ysize-50:
					y = 5
					elcount[page][0] = elcount[page][0]+1
					elcount[page][1] = 0
				x = 40 + elcount[page][0]*320
				newlabel.set_position(x,y) 
				self.pages[page].append(newlabel)
				elcount[page][1] = elcount[page][1] + 1
		config_file.close()

	def set_page(self, n):
		# remove elements on current page
		# do not remove if this is the first page set
		if self.curpage != -1:
			for el in self.pages[self.curpage]:
				el.hide()
				self.stage.remove(el)
		for el in self.pages[n]:
			self.stage.add(el)
			el.show()
		self.curpage = n

	def next_button_clicked(self, source, cond):
		# go to next page
		self.set_page((self.curpage + 1) % self.numpages)

	def key_pressed(self, source, cond):
		# reload the display
		# TODO: fix memory waste... elements are never deleted!
		if cond.keyval == 114:
		# hide all elements
			for page in self.pages:
				for el in page:
					el.hide()
					# also remove elements currently on stage
					if el in self.pages[self.curpage]:
						self.stage.remove(el)
			# rebuild
			self.build_display(self.configfile)
			# show page 0
			self.set_page(0)
		
	def start(self):
		self.stage.show_all()
		clutter.main()

if __name__ == "__main__":
	g = gui()
	try:
		g.start()
	except (KeyboardInterrupt):
		pass
