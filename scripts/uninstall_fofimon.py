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

# File:   uninstall_fofimon.py

# Removes all files installed by the install_fofimon.py script.

import sys
import os
import subprocess

def main():
	import argparse
	oParser = argparse.ArgumentParser("Removes all files created by install_fofimon.py")
	oParser.add_argument("-r", "--remove-builds", help="Remove build folders", action="store_true"\
						, default=False, dest="bRemoveBuilds")
	oParser.add_argument("-y", "--no-prompt", help="No prompt comfirmation", action="store_true"\
						, default=False, dest="bNoPrompt")
	oParser.add_argument("--destdir", help="install prefix (default=/usr/local)", metavar='DESTDIR'\
						, default="/usr/local", dest="sDestDir")
	oParser.add_argument("--no-sudo", help="don't use sudo to uninstall", action="store_true"\
						, default=False, dest="bDontSudo")
	oArgs = oParser.parse_args()

	sDestDir = os.path.abspath(oArgs.sDestDir)

	sScriptDir = os.path.dirname(os.path.abspath(__file__))
	os.chdir(sScriptDir)
	os.chdir("..")

	print("Uninstall from dir: " + sDestDir + "   Remove build folders: " + str(oArgs.bRemoveBuilds))

	if not oArgs.bNoPrompt:
		print("Uninstall from dir: " + sDestDir + "   Remove build folders: " + str(oArgs.bRemoveBuilds))

	while not oArgs.bNoPrompt:
		sAnswer = input("Are you sure? (yes/no) >").strip()
		if sAnswer.casefold() == "yes":
			break
		elif sAnswer.casefold() == "no":
			return #-----------------------------------------------------------

	if oArgs.bDontSudo:
		sSudo = ""
	else:
		sSudo = "sudo"

	subprocess.check_call("{} rm -r -f {}/bin/fofimon".format(sSudo, sDestDir).split())
	#subprocess.check_call("{} rm -r -f {}/share/fofimon".format(sSudo, sDestDir).split())
	#subprocess.check_call("{} rm -r -f {}/share/icons/hicolor/24x24/apps/fofimon.png".format(sSudo, sDestDir).split())
	#subprocess.check_call("{} rm -r -f {}/share/icons/hicolor/32x32/apps/fofimon.png".format(sSudo, sDestDir).split())
	#subprocess.check_call("{} rm -r -f {}/share/icons/hicolor/48x48/apps/fofimon.png".format(sSudo, sDestDir).split())
	#subprocess.check_call("{} rm -r -f {}/share/icons/hicolor/scalable/apps/fofimon.svg".format(sSudo, sDestDir).split())
	#subprocess.check_call("{} rm -r -f {}/share/applications/fofimon.desktop".format(sSudo, sDestDir).split())

	if oArgs.bRemoveBuilds:
		os.chdir("..")
		subprocess.check_call("{} rm -r -f fofimon/build".format(sSudo).split())


if __name__ == "__main__":
	main()

