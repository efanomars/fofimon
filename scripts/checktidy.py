#!/usr/bin/env python3

#  Copyright © 2018  Stefano Marsili, <stemars@gmx.ch>
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

# File:   checktidy.py

# Use this command to check the fofimon project with clang-tidy.

import sys
import os
import subprocess

def callTidy(sSubDir):
	#
	#os.chdir(sSubDir)

	print("clang-tidy started for project: " + sSubDir)

	sTidyOutput = "build/" + sSubDir + "_tidy.log"
	sCmd = ("clang-tidy -checks='.*' -p build/compile_commands.json  src/*.cc"
			" >" + sTidyOutput)
	subprocess.check_call(sCmd, shell=True)
	#
	if os.path.getsize(sTidyOutput) > 0:
		raise RuntimeError("Error: " + sTidyOutput + " not empty")

	print("clang-tidy finished project " + sSubDir + ". Output file: " + sTidyOutput)

	os.chdir("..")

def main():

	import argparse
	oParser = argparse.ArgumentParser(description="Clang tidy")
	oArgs = oParser.parse_args()

	sScriptDir = os.path.dirname(os.path.abspath(__file__))

	os.chdir(sScriptDir)
	os.chdir("..")
	#
	callTidy("fofimon")

if __name__ == "__main__":
	main()

