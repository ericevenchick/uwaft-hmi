import clutter
import canparse
# this file defines all of the CAN display elements
# which are used by the gui

# default font and colors
globalfont = "Helvetica 25"
globalfontcolor = clutter.Color(255,255,255)

class can_label(clutter.Text):
	def __init__(self, args):
		clutter.Text.__init__(self)
		self.original_text = args.split(',')[0]
		self.set_text(self.original_text)
		self.handler = args
		self.set_font_name(globalfont)
		self.set_color(globalfontcolor)
		self.set_size(25,100)
	def can_update(self, data):
		try:
			# does the handler exist?
			if hasattr(canparse, self.handler):
				# call the handler
				newtext = str(getattr(canparse, self.handler)(data))
			else:
				# treat this like a static label
				newtext = ""
		except IndexError:
			# no data to retrieve, ignore
			return
		if newtext != "None":
			self.set_text(self.original_text + " " + newtext)

class can_number(clutter.Text):
	def __init__(self, args):
		clutter.Text.__init__(self)
		self.set_font_name(globalfont)
		self.set_color(globalfontcolor)
		self.set_size(25,100)
		self.units = ""
		arglist = args.strip().split(',')
		self.original_label = arglist[0]
		self.units = arglist[1]
		self.canbus = int(arglist[2])
		self.canid = int(arglist[3])
		self.sigstartbit = int(arglist[4])
		self.siglength = int(arglist[5])
		self.scalefactor = float(arglist[6])
		self.set_text(self.original_label)

	def can_update(self, data):
		msgbus = int(data.strip().split('-')[0])
		msgid = int(data.strip().split('-')[1])
		if self.canid == msgid and self.canbus == msgbus:
			try:
				# get the value and multiply by the scale
				value = canparse.get_bits(data, self.sigstartbit, self.siglength)*self.scalefactor
			except IndexError:
				# no data was given
				return
			newtext = "%.1f" % value
			self.set_text(self.original_label + " " + newtext + " " + self.units)

class can_bool(clutter.Texture):
	def __init__(self, args):
		clutter.Texture.__init__(self, args)
		arglist = args.strip().split(',')
		self.set_size(25, 25)
		self.canbus = int(arglist[1]) 
		self.canid = int(arglist[2]) 
		self.sigbit = int(arglist[3])
		self.true_tex = clutter.cogl.texture_new_from_file('/home/eric/uwaft-hmi/gui/img/bool-true.svg', clutter.cogl.TEXTURE_NO_SLICING, clutter.cogl.PIXEL_FORMAT_ANY)
		self.false_tex = clutter.cogl.texture_new_from_file('/home/eric/uwaft-hmi/gui/img/bool-false.svg', clutter.cogl.TEXTURE_NO_SLICING, clutter.cogl.PIXEL_FORMAT_ANY)
		self.set_cogl_texture(self.false_tex)
	def can_update(self, data):
		msgbus = int(data.strip().split('-')[0])
		msgid = int(data.strip().split('-')[1])
		if self.canid == msgid and self.canbus == msgbus:
			try:
				value = canparse.get_bits(data, self.sigbit, 1) 
			except IndexError:
				return
			if value:
				self.set_cogl_texture(self.true_tex)
			else:
				self.set_cogl_texture(self.false_tex)
