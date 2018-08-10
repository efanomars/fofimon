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

# File:   sourcepackage.py

# Creates a source package for the fofimon project.

import sys
import os
import subprocess
from datetime import date

def main():

	import argparse
	oParser = argparse.ArgumentParser(description="Create source tar.gz")
	oParser.add_argument("--only-cli", help="only cli sources", action="store_true"\
						, default=False, dest="bOnlyCli")
	oArgs = oParser.parse_args()

	aExclPatterns = ["build*", "configure", "nbproject*", ".project", ".cproject", ".settings", "core"]

	sExcludes = ""
	for sExclPattern in aExclPatterns:
		sExcludes += " --exclude=" + sExclPattern

	if oArgs.bOnlyCli:
		sExcludes += " --exclude=gui*"
		sExcludes += " --exclude=icons*"
		sExcludes += " --exclude=applications*"
		sExcludes += " --exclude=LICENSE.3rdparty"

	sScriptDirPath = os.path.dirname(os.path.abspath(__file__))
	sSourceDirPath = os.path.dirname(sScriptDirPath)
	sSourceDir = os.path.basename(sSourceDirPath)
	sParentDirPath = os.path.dirname(sSourceDirPath)

	os.chdir(sParentDirPath)


	oToday = date.today()
	sToday = ("000" + str(oToday.year))[-4:] + ("0" + str(oToday.month))[-2:] + ("0" + str(oToday.day))[-2:]

	#print("sSourceDir=" + sSourceDir)
	#print("sParentDirPath=" + sParentDirPath)
	#print("sExcludes=" + sExcludes)
	#print("sToday=" + sToday)

	sCmd = ("tar -zcf  fofimon-{}.tar.gz -v"
							" {}"
							" --exclude=.git"
							" --exclude=stuff"
							" --exclude=.metadata"
							" --exclude=core"
							" {}").format(sToday, sExcludes, sSourceDir)
	print(sCmd)

	subprocess.check_call(sCmd.split())


if __name__ == "__main__":
	main()

