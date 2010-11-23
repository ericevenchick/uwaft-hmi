#!/usr/bin/python
# Main entry point for HMI. Currently only launches GUI
from gui import gui 
g = gui.gui("/home/eric/uwaft-hmi/canmsg", "/home/eric/uwaft-hmi/gui/config")
g.start()
