#! /usr/bin/env python

import sys, string

import leo_channel
import leo_vchannel

import yappy.parser
from yappy.parser import *

yappy.parser._DEBUG = 0

class LeoCfg(Yappy):
    """Define a Leoparse-compatible Yappy parser."""
    def __init__(self, no_table=1, table='leo.tab'):

        grammar	= grules([
            ("F -> H",			self.F),
            ("H -> H id : O ;",		self.HH),
            ("H -> id : O ;",		self.H),
            ("L -> L , O",		self.LL),
            ("L -> O",			self.L),
            ("O -> { H }",		self.OH),
            ("O -> id",			self.OId),
            ("O -> id M",		self.OIdM),
            ("O -> int",		self.OInt),
            ("O -> R",			self.OR),
            ("O -> ( L )",		self.OL),
            ("M -> [ L ]",		self.ML),
            ("R -> int .. int",		self.R)
            ])
        
        tokenize = [("\s+",                             ""),
                    ("[-_a-zA-Z][-._a-zA-Z0-9]*",	lambda x: ("id", x)),
                    ("[0-9]+",				lambda x: ("int",x)),
                    ("\,",				lambda x: (x,x)    ),
                    ("\:",				lambda x: (x,x)    ),
                    ("\;",				lambda x: (x,x)    ),
                    ("\{|\}", 				lambda x: (x,x)    ),
                    ("\[|\]", 				lambda x: (x,x)    ),
                    ("\.\.", 				lambda x: (x,x)    ),
                    ("\(|\)", 				lambda x: (x,x)    )
                    ]

        Yappy.__init__(self, tokenize, grammar, table, no_table)

    def F(self,list,context=None):
        return list[0]
    
    def HH(self,list,context=None):
        list[0][list[1]] = list[3]
        return list[0]
    
    def H(self,list,context=None):
        return { list[0]: list[2] }
    
    def LL(self,list,context=None):
        return list[0] + [ list[2] ]
    
    def L(self,list,context=None):
        return [ list[0] ]
    
    def OH(self,list,context=None):
        return list[1]
    
    def OInt(self,list,context=None):
        return int(list[0])
    
    def OId(self,list,context=None):
        return list[0]
    
    def OIdM(self,list,context=None):
        return (list[0], list[1])
    
    def OR(self,list,context=None):
        return list[0]
    
    def OL(self,list,context=None):
        return list[1]
        
    def ML(self,list,context=None):
        return list[1]
        
    def R(self,list,context=None):
        return xrange(int(list[0]), 1+int(list[2]))
        
def cfgfile_parse(filename):
    """Parse a Leoparse configuration file."""
    d = LeoCfg(1)
    return d.inputfile(filename)

def netcfg_process(s):
    """Process a network configuration file."""
    netcfg	= cfgfile_parse(s.options.net)
    net_list	= netcfg['networks']
    if type(net_list) is not list:
        net_list = [net_list]

    s.net_dict	= {}
    for net in net_list:
        s.net_dict[net['name']] = net

def appcfg_process(s):
    """Process an application configuration file."""
    filename	= s.args.pop(0)
    appcfg	= cfgfile_parse(filename)
    s.appdata	= appcfg['application']

    if s.appdata.has_key('name') and s.options.appli == None:
        s.options.appli = s.appdata['name']

    if s.appdata.has_key('flavor') and s.options.flavor == None:
        s.options.flavor = s.appdata['flavor']

    s.appnetdata = s.appdata['networks']

    if s.appnetdata.has_key('include') and s.options.net == None:
        s.options.net = s.appnetdata['include']

    netcfg_process(s)

    leo_channel.channels_process(s)
    leo_vchannel.vchannels_process(s, False)
    leo_vchannel.vchannels_process(s, True)
