#!/bin/bash

# Usage: 
# example app path 				[string] E.g. ./Example_2_Load_Measurement.exe for windows or ./Example_2_Load_Measurement for linux	
# user settings directory 		[string] Default is ../settings
# sessionfile (.cu3s)			[string] Default is ../SDK_Training_Example_Data/single_raw.cu3s (demo data, Ultris X20)

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    ##########LINUX##########
	./Example_2_Load_Measurement											\
	"../settings"					\
	"../SDK_Training_Example_Data/single_raw.cu3s"	\
	
else
    ##########WINDOWS##########
	./Example_2_Load_Measurement.exe 										\
	"../settings"					\
	"../SDK_Training_Example_Data/single_raw.cu3s"	\
	
fi
