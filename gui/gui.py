import clutter
import time
import os
import glib
import canparse
import bar
xsize = 750
ysize = 500

next_button_tex = clutter.cogl.texture_new_from_file('/home/eric/uwaft-hmi/gui/img/next-button.svg', clutter.cogl.TEXTURE_NO_SLICING, clutter.cogl.PIXEL_FORMAT_ANY)

batt_temp_tex = clutter.cogl.texture_new_from_file('/home/eric/uwaft-hmi/gui/img/batt-temp.svg', clutter.cogl.TEXTURE_NO_SLICING, clutter.cogl.PIXEL_FORMAT_ANY)

class can_label(clutter.Text):
	def __init__(self):
		clutter.Text.__init__(self)
		self.handler = ""
		self.original_text = ""

class can_bool(clutter.Texture):
	def __init__(self):
		clutter.Texture.__init__(self)
		self.handler = ""
		self.state = False
		self.true_tex = clutter.cogl.texture_new_from_file('/home/eric/uwaft-hmi/gui/img/bool-true.svg', clutter.cogl.TEXTURE_NO_SLICING, clutter.cogl.PIXEL_FORMAT_ANY)
		self.false_tex = clutter.cogl.texture_new_from_file('/home/eric/uwaft-hmi/gui/img/bool-false.svg', clutter.cogl.TEXTURE_NO_SLICING, clutter.cogl.PIXEL_FORMAT_ANY)

class gui:
	# handle data from the canbus
	def update(self, source, cond):
		# get new data from canbus
		# and remove the newline
		data = source.readline()
		data = data.strip()
		
		#enable for debug
		#print data
		
		# get the message id so we can determine
		# what should be updated
		canid = -1
		try:
			canid = int(data.split('-')[0])
		except ValueError:
			pass
		try:
				# handle can_labels
				for page in self.pages:
					for el in page:
						if el.__class__ == can_label:
							value = getattr(canparse, el.handler)(data)
							el.set_text(el.original_text + " " + str(value))
						elif el.__class__ == can_bool:
							value = getattr(canparse, el.handler)(data)
							if value > 0:
								el.state = True
								el.set_cogl_texture(el.true_tex)
							else:
								el.state = False
								el.set_cogl_texture(el.false_tex)
				# display H2 Alarm
				h2_alarm = canparse.get_h2_alarm(data)
				if h2_alarm:
					self.h2_alarm_label.set_text("H2!")
					self.stage.set_color(clutter.Color(255,0,0))
				else:
					self.h2_alarm_label.set_text("")
					self.stage.set_color(clutter.Color(0,0,0))
		except (IndexError):
			pass
		# return true to keep handling
		# input from the canbus pipe
		return True
	def close(self):
		canbus.close()
		clutter.main_quit()
	def __init__(self, canbusfile, configfile):
		self.configfile = configfile

		# Setup info pages
		self.build_display(configfile)

		# Open the can bus pipe
		self.canbus = open(canbusfile, "r+")
		
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
		# set up to watch the canbus with handler
		# function self.update
		glib.io_add_watch(self.canbus, glib.IO_IN, self.update)
	
	def build_display(self,config):
		config_file = open(config, "r+")
		# get the number of pages (first line of file)
		self.numpages = int(config_file.readline())
		# The current page number (-1 for none)
		self.curpage = -1
		# store the pages and number of elements on each page
		self.pages = []
		elcount = []
		for i in range(1,self.numpages+1):
			self.pages.append([])
			elcount.append([0,0])
		
		# create the elements
		for line in config_file.readlines():
			page,disptype,text,handler= line.strip().split(',')	
			page = int(page)

			# boolean display
			if disptype == 'b':
				newlabel = can_label()
				newlabel.set_text(text)
				newlabel.set_size(25,100)
				newlabel.set_font_name("Helvetica 25")
				newlabel.set_color(clutter.Color(255,255,255))
				y = 5+40*elcount[page][1]
				if y > ysize-50:
					y = 5
					elcount[page][0] = elcount[page][0]+1
					elcount[page][1] = 0
				x = 40 + elcount[page][0]*320
				newlabel.set_position(x,y) 
				newlabel.handler = "none"
				newlabel.original_text = text
				self.pages[page].append(newlabel)

				newbool = can_bool()
				newbool.set_size(25, 25)
				newbool.set_position(x + 220, y+7)
				newbool.set_cogl_texture(newbool.false_tex)
				newbool.handler = handler
				newbool.state = False
				self.pages[page].append(newbool)
				
				elcount[page][1] = elcount[page][1] + 1
			# text display
			if disptype == 't':
				newlabel = can_label()
				newlabel.set_text(text)
				newlabel.set_size(25,100)
				newlabel.set_font_name("Helvetica 25")
				newlabel.set_color(clutter.Color(255,255,255))
				y = 5+40*elcount[page][1]
				if y > ysize-50:
					y = 5
					elcount[page][0] = elcount[page][0]+1
					elcount[page][1] = 0
				x = 40 + elcount[page][0]*320
				newlabel.set_position(x,y) 
				newlabel.handler = handler
				newlabel.original_text = text
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
		self.set_page((self.curpage + 1) % self.numpages)
	def key_pressed(self, source, cond):
		# reload the display
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
