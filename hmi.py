#!/usr/bin/python
# Main entry point for HMI. Currently only launches GUI
from gui import gui 
g = gui.gui("/home/eric/uwaft-hmi/canmsg")
g.start()
