import candisplay

# this is where custom pages can be defined
# each one is a function with the same name as the page
# each takes one argument, which is the page to append its elements to
# to use the page, edit the configuration file adding a line of the form
# page#,p,pagename

# Home Page
def home(page):
	# Speedometer Display
	speed = candisplay.can_number(",\nkm/h,0,0,0,0,0")
	speed.set_position(700,422)
	speed.set_size(100,100)
	speed.set_font_name("Futura 50")
	page.append(speed)

	# State of Charge Display
	soc = candisplay.can_number("SOC: ,%,0,0,0,0,0")
	soc.set_position(10,10)
	soc.set_size(100,100)
	page.append(soc)

	# Some Other Display
	soc = candisplay.can_number("Something: ,%,0,0,0,0,0")
	soc.set_position(10,80)
	soc.set_size(100,100)
	page.append(soc)

	# Dial Display 
	dial = candisplay.can_dial(1,241,8,8,0.392157)
	dial.set_position(25,210)
	page.append(dial)
	diallabel = candisplay.can_label("H2 level, none")
	diallabel.set_position(80,230)
	diallabel.set_font_name("Helvetica 15")
	page.append(diallabel)



