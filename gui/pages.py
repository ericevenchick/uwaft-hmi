import candisplay

# this is where custom pages can be defined
# each one is a function with the same name as the page
# each takes one argument, which is the page to append its elements to
# to use the page, edit the configuration file adding a line of the form
# page#,p,pagename

# useless test
def test(page):
	print "doing test page!"
	label = candisplay.can_label("Hello!")
	label.set_position(50,50)
	label.set_size(100,100)
	page.append(label)
