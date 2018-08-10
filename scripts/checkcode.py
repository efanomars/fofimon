#!/usr/bin/env python3

#  Copyright © 2017  Stefano Marsili, <stemars@gmx.ch>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, see <http://www.gnu.org/licenses/>

# File:   checkcode.py

# Checks whether there are still debug comments in the source code.

import sys
import os
import re

oPattStdCout = re.compile("^std::cout <<")

def checkSourceFile(sPathFile):
	if "/build/" in sPathFile:
		return
	if "/googletest/" in sPathFile:
		return
	if "/gtest/" in sPathFile:
		return
	print("Checking: " + sPathFile)
	try:
		oF = open(sPathFile, 'r')
	except PermissionError:
		return
	for sLine in oF:
		if oPattStdCout.search(sLine):
			raise RuntimeError("Found '^std::cout <<' in file: " + sPathFile)

def checkCurDir():
	aEntries = os.listdir()
	for sEntry in aEntries:
		if sEntry[:1] == '.':
			# don't consider hidden files
			continue
		sPathEntry = os.path.abspath(sEntry)
		if os.path.islink(sPathEntry):
			# don't follow symbolic links
			continue
		elif os.path.isdir(sPathEntry):
			os.chdir(sEntry)
			checkCurDir()
			os.chdir("..")
		else:
			(sRoot, sExt) = os.path.splitext(sEntry)
			if (sExt == ".cc") or (sExt == ".cpp") or (sExt == ".cxx") or (sExt == ".c++")\
					or (sExt == ".h") or (sExt == ".hh") or (sExt == ".hpp") or (sExt == ".hxx"):
				checkSourceFile(sPathEntry)

def main():

	sScriptDir = os.path.dirname(os.path.abspath(__file__))

	os.chdir(sScriptDir)
	os.chdir("..")

	checkCurDir()

if __name__ == "__main__":
	main()
