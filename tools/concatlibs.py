import os
import sys

##############
# NOTE: You will need to build boost
# On windows, use the following command from the visual studio command
# prompt (after running boostrap.bat)
#
# bjam --build-dir=c:\boost --build-type=complete --toolset=msvc-9.0 address-model=64 architecture=x86 --with-system
##############

currentPath = os.getcwd()

config = "Debug"
boostLocation = "C:/boost"

# Main list of libraries to be concatinated into the final library 
# NOTE: For non-windows environments, the .lib is replaced with .o below
#
# Remove any libries from this list that you would rather link manually (separately)
inputLibs = [ 
	# Main node.js library
	currentPath+'/'+config+"/lib/node.lib", 

	# v8
	currentPath+'/build/'+config+"/lib/v8_base.lib", 
	currentPath+'/build/'+config+"/lib/v8_nosnapshot.lib", 
	currentPath+'/build/'+config+"/lib/v8_snapshot.lib",

	# libuv
	currentPath+'/'+config+"/lib/libuv.lib", 

	# Other direct dependencies of node
	currentPath+'/'+config+"/lib/cares.lib", 
	currentPath+'/'+config+"/lib/http_parser.lib", 
	currentPath+'/'+config+"/lib/openssl.lib", 
	currentPath+'/'+config+"/lib/zlib.lib", 

	# Boost
	boostLocation+"/boost/bin.v2/libs/system/build/msvc-9.0/"+config+"/address-model-64/architecture-x86/link-static/runtime-link-static/libboost_system-vc90-sgd-1_52.lib"
]

inputLibString = ""
inputOString = ""

# Build our list of input libraries for windows (.lib)
for lib in inputLibs:
    inputLibString += lib + " "

# Build an equivelant list for non-windows (.o)
for lib in inputLibs:
	lib.replace(".lib", ".o")
	inputOString += lib + " "

# Concatinate!
if sys.platform == 'win32':
	os.system('lib.exe /OUT:'+currentPath+'/'+config+'/node.lib ' + inputLibString)
else:
	os.system('ar rcs '+currentPath+'/'+config+'/node.a ' + inputOString)
