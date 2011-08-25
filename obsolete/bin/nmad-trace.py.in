#!/usr/bin/python2.5

# PM2: Parallel Multithreaded Machine
# Copyright (C) 2007 "the PM2 team" (see AUTHORS file)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#

class NM_Event:
    def __init__(self, list):
	self.previous_length	= int(list.pop(0), 16)
        self.num	= int(list.pop(0))
        self.pid	= int(list.pop(0))
        self.time	= int(list.pop(0))
        self.tid	= int(list.pop(0))
        self.code	= int(list.pop(0))
        self.nb_args	= int(list.pop(0))
        self.args	= list

    def __repr__(self):
        return '%08x: %016x - %02x[%04x]: %04x' % (self.num, self.time, self.pid, self.tid, self.code)

# end NM_Event

class NM_Event_file:
    def __init__(self, filename):
        self.filename	= filename
        self.file	= open(self.filename, "rb")

        self.current_event		= None
	self.current_event_start	= self.file.tell()
	self.eof			= False
	
    def read_event(self):
        buf	= self.file.readline()
	if buf == '':
		return None

        buf	= buf.rstrip('\n')
        list	= buf.split(',')
	event	= NM_Event(list)

	return event
    	
    def previous_event(self):
    	if self.current_event == None:
		return None

	if self.eof:
		self.eof	= False
		return self.current_event

	if self.current_event_start == 0:
		self.file.seek(0)
		self.current_event	= None
		return None
		
	self.file.seek(self.current_event_start - self.current_event.previous_length)
	
	start	= self.file.tell()
	event	= self.read_event()	

	self.current_event_start	= start
        self.current_event		= event

        return event

    def next_event(self):
    	if self.eof:
		return None

	start	= self.file.tell()
	event	= self.read_event()	

	if event == None:
		self.eof	= True
	else:	
		self.current_event_start	= start
       		self.current_event		= event

        return event


# end NM_Event_file

# -- main --  example of use
ev_file	= NM_Event_file('dummy.raw')

while True:
	event	= ev_file.next_event()
	if event == None:
		break

	print event

print '--------------------'

while True:
	event	= ev_file.previous_event()
	if event == None:
		break

	print event

print '--------------------'

print ev_file.next_event()

#

