import os
import sys

currentPath = os.getcwd()

if sys.platform == 'win32':
	os.system('lib.exe /OUT:'+currentPath+'/Release/node.lib  '+currentPath+'/Release/lib/cares.lib '+currentPath+'/Release/lib/http_parser.lib '+currentPath+'/Release/lib/libuv.lib '+currentPath+'/Release/lib/node.lib '+currentPath+'/Release/lib/openssl.lib '+currentPath+'/Release/lib/zlib.lib')
else:
	os.system('ar rcs '+currentPath+'/Release/node.a '+currentPath+'/Release/lib/cares.o '+currentPath+'/Release/lib/http_parser.o '+currentPath+'/Release/lib/libuv.o '+currentPath+'/Release/lib/node.o '+currentPath+'/Release/lib/openssl.o '+currentPath+'/Release/lib/zlib.o')
