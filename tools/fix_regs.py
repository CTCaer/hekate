import re

def parse_defs(fname):
	f = open(fname, "r")
	lines = f.readlines()
	f.close()
	res = {}
	for l in lines:
		p = [str(_.strip()) for _ in l.strip().split(" ", 1)]
		res[int(p[1], 16)] = p[0]
	return res

mc = parse_defs("mc.def")
emc = parse_defs("emc.def")

f = open(sys.argv[1], "r")
buf = f.read()
f.close()

def fix(m):
	what = m.groups()[0]
	off = int(m.groups()[1], 16)
	if what == "MC":
		if off in mc:
			return "MC({0})".format(mc[off])
	elif what == "EMC":
		if off in emc:
			return "EMC({0})".format(emc[off])
	return "{0}(0x{1:X})".format(what, off)

buf = re.sub(r'([A-Z]+)\(0x([0-9a-fA-F]+)\)', fix, buf)

f = open(sys.argv[2], "w")
f.write(buf)
f.close()
