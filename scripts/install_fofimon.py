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

# File:   install_fofimon.py

# Compiles and installs the fofimon application.

import sys
import os
import subprocess

def main():
	import argparse
	oParser = argparse.ArgumentParser()
	oParser.add_argument("-b", "--buildtype", help="build type (default=Release)"\
						, choices=['Debug', 'Release', 'MinSizeRel', 'RelWithDebInfo']\
						, default="Release", dest="sBuildType")
	oParser.add_argument("-t", "--tests", help="build tests (default=Cache)", choices=['On', 'Off', 'Cache']\
						, default="Cache", dest="sBuildTests")
	oParser.add_argument("--destdir", help="install prefix (default=/usr/local)", metavar='DESTDIR'\
						, default="/usr/local", dest="sDestDir")
	oParser.add_argument("--no-sudo", help="don't use sudo to install", action="store_true"\
						, default=False, dest="bDontSudo")
	#oParser.add_argument("--no-icons", help="don't install icons (implies --no-launcher)", action="store_true"\
						#, default=False, dest="bNoIcons")
	#oParser.add_argument("--no-launcher", help="don't install launcher (which needs gksu)", action="store_true"\
						#, default=False, dest="bNoLauncher")
	oArgs = oParser.parse_args()

	sDestDir = os.path.abspath(oArgs.sDestDir)

	sScriptDir = os.path.dirname(os.path.abspath(__file__))
	#print("sScriptDir:" + sScriptDir)
	os.chdir(sScriptDir)
	os.chdir("..")
	#
	sDestDir = "-D CMAKE_INSTALL_PREFIX=" + sDestDir
	#print("sDestDir:" + sDestDir)
	#
	sBuildType = "-D CMAKE_BUILD_TYPE=" + oArgs.sBuildType
	#print("sBuildType:" + sBuildType)

	sBuildTests = "-D BUILD_TESTING="
	if oArgs.sBuildTests == "On":
		sBuildTests += "ON"
	elif oArgs.sBuildTests == "Off":
		sBuildTests += "OFF"
	else:
		sBuildTests = ""
	#print("sBuildTests:" + sBuildTests)
	#
	if oArgs.bDontSudo:
		sSudo = ""
	else:
		sSudo = "sudo"

	#sInstallIcons = "-D STMM_INSTALL_ICONS="
	#if (oArgs.bNoIcons):
		#sInstallIcons += "OFF"
		#oArgs.bNoLauncher = True
	#else:
		#sInstallIcons += "ON"

	#sInstallLauncher = "-D STMM_INSTALL_LAUNCHER="
	#if (oArgs.bNoLauncher):
		#sInstallLauncher += "OFF"
	#else:
		#sInstallLauncher += "ON"

	if not os.path.isdir("build"):
		os.mkdir("build")

	os.chdir("build")

	#subprocess.check_call("cmake {} {} {} {} {} ..".format(sBuildType, sBuildTests, sDestDir, sInstallIcons, sInstallLauncher).split())
	subprocess.check_call("cmake {} {} {} ..".format(sBuildType, sBuildTests, sDestDir).split())
	subprocess.check_call("make $STMM_MAKE_OPTIONS", shell=True)
	subprocess.check_call("{} make install".format(sSudo).split())


if __name__ == "__main__":
	main()

