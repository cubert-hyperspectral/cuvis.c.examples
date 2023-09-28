#!/bin/bash

# Usage:
# example app path 				[string] E.g. ./04_changeDistance.exe for windows or ./04_changeDistance for linux
# user settings directory 		[string] Default is ../../../sample_data/set_examples/settings
# sessionfile (.cu3s)			[string] Default is ../../../sample_data/set_examples/set0_single/single.cu3s
# new distance 					[int   ] distance reference in mm
# name of export directory 		[string] The name of your output folder realtive to the example app path

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    ##########LINUX##########
	./04_changeDistance													\
	"../../../sample_data/set_examples/settings"						\
	"../../../sample_data/set_examples/set0_single/single.cu3s" 		\
	1000																\
	"export"															\

else
    ##########WINDOWS##########
	./04_changeDistance.exe 											\
	"../../../sample_data/set_examples/settings"						\
	"../../../sample_data/set_examples/set0_single/single.cu3s" 		\
	1000																\
	"export"															\

fi
