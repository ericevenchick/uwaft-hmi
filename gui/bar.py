import clutter
class bar :
	def __init__(self, target, xpos, ypos, width=6, height=25, 
		color=clutter.Color(255,255,255), segs=10):
		self.rects = []
		self.target = target
		self.segs = segs
		# create and initialize each rectangle
		for i in range(0,segs):
			rect = clutter.Rectangle()
			rect.set_size(width,height)
			rect.set_color(color)
			rect.set_position(xpos + (width+5)*i, ypos)
			self.target.add(rect)
			self.rects.append(rect)
	def show(self):
		for i in range(0,self.segs):
			self.rects[i].show()
			self.target.add(self.rects[i])
	def hide(self):
		for i in range(0,self.segs):
			self.target.remove(self.rects[i])
			self.rects[i].hide()
	def set_segs(self, segs):
		self.hide()
		self.segs = segs
		self.show()
