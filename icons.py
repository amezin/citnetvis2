#!/usr/bin/env python
import re, io, os
regex = re.compile("""QIcon\s*::\s*fromTheme\s*\(\s*\"([a-zA-Z-]*)\"\s*\)""")
icons = set()
for file in os.listdir("."):
	if not os.path.isfile(file):
		continue
	if not file.endswith(".cpp"):
		continue
	for line in io.open(file, "r"):
		for m in re.finditer(regex, line):
			icons.add(m.group(1) + ".png")

rc = io.open("icons.qrc", "w")

rc.write("""<RCC>
  <qresource prefix="themes">
      <file>oxygen/index.theme</file>
""")

def printlist(dir):
	for item in os.listdir(dir):
		fullpath = os.path.join(dir, item)
		if os.path.isdir(fullpath):
			printlist(fullpath)
			continue
		if os.path.basename(fullpath) not in icons:
			continue
		rc.write("      <file>%s</file>\n" % (fullpath))

printlist(os.path.join("oxygen-icons", "oxygen"))

rc.write("""  </qresource>
</RCC>
""")
