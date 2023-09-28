#!/bin/bash

# Usage:
# example app path 				[string] E.g. ./06_recordVideo.exe for windows or ./06_recordVideo for linux	
# user settings directory 		[string] Default is ../../../sample_data/set_examples/settings
# sessionfile		 			[string] Default is ../../../sample_data/set_examples/set0_single/single.cu3s
# name of recording	directory	[string] The name of your output folder realtive to the example app path
# exposure time 				[int   ] exposure (integration) time in ms 
# auto exposure					[bool  ] 1 is "on", 0 is "off"; if "on" the aquisition will start with the given exposure time and then adjust on the fly 
# target fps					[double] Frames Per Second that shall be recorded. 

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    ##########LINUX##########
	./07_recordVideoFromSessionfile											\
	"../../../sample_data/set_examples/settings" 							\
	"../../../sample_data/set_examples/set0_single/single.cu3s"		\
	"video01"																\
	100																		\
	0																		\
	2																		\
	
else
    ##########WINDOWS##########
	./07_recordVideoFromSessionfile.exe 									\
	"../../../sample_data/set_examples/settings"							\
	"../../../sample_data/set_examples/set0_single/single.cu3s"		\
	"video01"																\
	100																		\
	0																		\
	2																		\
	
fi
