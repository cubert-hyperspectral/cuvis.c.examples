#!/bin/bash

# Usage:
# example app path 				[string] E.g. ./Example_8_Change_Distance.exe for windows or ./Example_8_Change_Distance for linux
# user settings directory 		[string] Default is ../settings
# sessionfile (.cu3s)			[string] Default is ../SDK_Training_Example_Data/single_raw.cu3s (demo data, Ultris X20)
# new distance 					[int   ] distance reference in mm
# name of export directory 		[string] The name of your output folder relative to the example app path

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    ##########LINUX##########
	./Example_8_Change_Distance													\
	"../settings"						\
	"../SDK_Training_Example_Data/single_raw.cu3s" 		\
	1000																\
	"export"															\

else
    ##########WINDOWS##########
	./Example_8_Change_Distance.exe 											\
	"../settings"						\
	"../SDK_Training_Example_Data/single_raw.cu3s" 		\
	1000																\
	"export"															\

fi
