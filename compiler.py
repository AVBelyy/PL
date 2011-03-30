#!/usr/bin/env python

#
#	(C) Anton Belyy, 2011
#

import re
from sys import argv, exit
from time import time
from os.path import isfile, abspath, exists, join, basename, getsize
from os import environ, pathsep
from string import whitespace

# constants
opcodes = (
	("nop",		0x00), #
	("add",		0x01), #
	("sub", 	0x02), #
	("mul",		0x05), #
	("div",		0x06), #
	("xor",		0x07), #
	("or",		0x08), #
	("and",		0x09), #
	("shl", 	0x0a), #
	("shr", 	0x0b), #
	("goto",	0x0c), #
	("inc",		0x0d), #
	("dec",		0x0e), #
	("if",		0x0f), #
	("mov",		0x10), #
	("call",	0x11), #
	("ret",		0x12), #
	("push",	0x13), #
	("movf",	0x14),
	("int",		0x15), #
	("mod",		0x16), #
	("pid",		0x17)  #
)

curLine, curSection, sectionLength, sections = 0, "", {"code": 0, "data": 0}, {"code": [], "data": []}
reservedSections = ("header", "data", "const", "import", "export", "code")
header = {"name": "", "library": False}
varTable, constTable, labelTable, procTable = [], [], [], []
structTable, structPtr = [], -1
exportTable, importTable, curLibrary = [], [], ""
includes, incLevel = [], 0
ifs, ifsLevel = [True], 0
constStrCounter = 0
path, outpath = environ["PATH"].split(pathsep), ""

class CompileError(Exception):
	def __init__(self, value):
		self.value = value
	def __str__(self):
		return repr(self.value)

def search_file(filename, paths):
	for path in paths:
		if exists(join(path, filename)):
			return abspath(join(path, filename))
	raise IOError, "file not found"

def splitln(ln):
	ln = re.sub('"[^"]*"', lambda x: x.group(0).replace(" ", "%%_SPACE_%%").replace("\t", "%%_TAB_%%"), ln)
	return [x.replace("%%_SPACE_%%", " ").replace("%%_TAB_%%", "\t") for x in re.split("\s+", ln)]

def encode(ln):
	for x in whitespace: ln = ln.replace(`x`[1:-1], x)
	return ln
	
def getcmd(name):
	global opcodes
	for x in opcodes:
		if x[0] == name: return x[1]

def getcint(cint):
	if not cint: return 0
	cint = cint.lower()
	for const in constTable:
		if cint == const[0]:
			cint = const[1]
			break
	if cint[0] == cint[-1] == "'":	return ord(encode(cint[1:-1])[0])
	if cint[:2] == "0x":			return int(cint, 16) & 0x7fff
	elif cint[:2] == "0b":			return int(cint, 2) & 0x7fff
	if cint[0] == cint[-1] == "'":	return ord(cint[1:-1]) & 0x7fff
	else:							return int(cint) & 0x7fff

def procid(proc):
	proc = proc.strip()
	if not proc: return
	counter = 0
	for x in procTable:
		if x["name"] == proc: return counter
		counter += 1

def varinfo(var):
	if var in [proc["name"] for proc in procTable]: # is procedure
		start = ((0x80 | header["library"]["static"] * 0x40 | (header["library"]["pid"] & 0x3f)) << 8) + (procid(var) & 0xff)
		return {"name": var, "type": "procedure", "start": start}
	for x in varTable:
		if x["name"] == var:
			info = x
			info["start"] = ((0x80 | header["library"]["static"] * 0x40 | (header["library"]["pid"] & 0x3f)) << 8) + (x["start"] & 0xff)
			return info

def checkop(operand, flags):
	if not operand["flags"] in flags:
		raise TypeError, "incorrect operand type"

def getop(op):
	operand = {"flags": 0, "value": 0}
	op = op.strip()
	for x in constTable:
		if x[0] == op.lower(): return getop(x[1])
	try:
		args = splitln(op[op.index("("):].strip()[1:-1])
		for x in args:
			if x.strip(): parse("PUSH " + x)
		op = op[:op.index("(")].strip()
		operand["flags"] = 4
	except: pass
	if op in [proc["name"] for proc in procTable]:
		return {"flags": operand["flags"] or 5, "value": 0, "id": procTable[procid(op)]["id"]}
	try:
		op_int = int(op)
	except:
		# if operand is non-int, check if it is variable pointer or register 
		# is it const string?
		if op[0] == op[-1] == '"':
			global constStrCounter
			op = getcstr(op)
			varTable.append({"name": "const_str" + str(constStrCounter), "type": "string", "start": sectionLength["data"], "size": len(op)})
			sectionLength["data"] += len(op)
			sections["data"].append(op)
			constStrCounter += 1
			return {"flags": 0, "value": sectionLength["data"] - len(op)}
		# ...or const char?
		if op[0] == op[-1] == "'": return {"flags": 0, "value": getcint(op)}
		# i think it's member of struct
		if op.find(".") != 1:
			try:
				struct, member = (re.sub("\\s*", "", token) for token in op.split("."))
				# search structure members
				info = [(s["sname"], s["start"]) for s in varTable if s["name"] == struct.lower()][0]
				members = [s for s in structTable if s["name"] == info[0].lower()][0]["members"]
				offset = 0
				for m in members: 
					if m[0] == member: 	
						return {"flags": 2 if m[1] == "int" else 1, "value": info[1] + offset}
					else:
						offset += m[2]
			except: pass
		if op[0] == "&": op, operand["flags"] = op[1:], 1
		if len(op) == 2 and op[0].lower() == "r" and ord(op[1]) in xrange(48, 58):
			return {"flags": (not operand["flags"]) + 3, "value": int(op[1])}
		for x in varTable:
			if x["name"] == op:
				if x["type"] == "int" and operand["flags"]: operand["flags"] = 2
				operand["value"] = x["start"]
				return operand
		if op[:2] not in ("0x", "0b"):
			raise NameError, "undefined variable '%s'" % op.strip()
	operand["value"] = getcint(op)
	return operand

def sizeof(t):
	if re.match("string", t):		# is it string?
		try:
			size = getop(re.search("(?<=string\[).*(?=\])", t).group(0))["value"]
			if size < 0:
				raise TypeError, "strings can't be negative or zero-length"
			else:
				return size
		except:
			raise TypeError, "can't get length of type '%s'" % t
	elif t == "int": return 2		# or integer?
	elif t in (struct["name"] for struct in structTable):		# or may be structure?
		members = [struct for struct in structTable if struct["name"] == t][0]["members"]
		return reduce(lambda x, y: x+y, [member[2] for member in members])
	else:		# so, what is it?
		raise TypeError, "can't get length of type '%s'" % t

def parsecond(cond):
	cond = cond.strip()
	if not cond: return
	operator = re.search("(=|!|<|>)=", cond)
	if not operator:
		if cond[0] == "!":
			cond, operator = cond[1:].strip() + "==0", "=="
		else:
			cond, operator = cond + "!=0", "!="		
	else: operator = operator.group(0)
	cond = cond.replace(operator, " %s " % operator)
	tokens = re.split("\\s+", cond, 2)
	counter = 0
	for x in ("==", "!=", "<=", ">="):
 		if operator == x:
			lvalue, rvalue = getop(tokens[0]), getop(tokens[2])
			checkop(lvalue, xrange(5))
			checkop(rvalue, xrange(5))
			return [counter, lvalue, rvalue]
		counter += 1

def getlabel(label):
	global labelTable
	try:
		label_int = int(label)
	except:
		for x in labelTable:
			if x["name"] == label and x["section"] == curSection:
				return x["offset"]

def parsefx(ln):
	try:	l, r = ln.index("("), ln.index(")")
	except:	return 0
	return {"name": ln[:l].strip(), "args": splitln(ln[l+1:r].strip())}

def getcstr(cstr): # parse const string in DATA section
	out = ""
	if cstr[0] == cstr[-1] == '"': out = encode(cstr[1:-1]) + chr(0)
	else:
		fx = parsefx(cstr.lower())
		try:	symbol = getcint(fx["args"][1])
		except:	symbol = 0
		if fx == 0 or fx["name"] not in ("fill"):
			raise CompileError, "const string '%s' have incorrect format" % cstr
		if fx["name"] == "fill":
			out = chr(symbol) * getcint(fx["args"][0])
	return out

def parse(ln):
	global curLibrary, curSection, structPtr
	global includes, incLevel
	global ifs, ifsLevel
	ln = ln.strip()
	if not ln: return
	if ln.lower() == "#else":
		ifs[ifsLevel] = not ifs[ifsLevel]
		return
	elif ln.lower() == "#endif":
		if ifsLevel == 0:
			raise CompileError, "unexpected '#endif'"
		ifsLevel -= 1
		ifs.pop()
		return
	if not ifs[ifsLevel]: return
	if ln[-1] == ":": # label
		curSection = ln[:-1].strip().lower()
		if curSection not in reservedSections: # is procedure
			if curSection in [proc["name"] for proc in procTable]: # already defined
				raise NameError, "duplicate definition of procedure '%s'" % curSection
			procTable.append({"global": False, "name": curSection, "id": len(procTable)})
		try:
			if sections[curSection]: pass
		except:
			sections[curSection] = []
			sectionLength[curSection] = 0		
	elif ln[:8].lower() == "#include":
		try:
			filename = re.split("\\s+", ln)[1]
		except:
			raise CompileError, "specify name of including file"
		if filename[0] + filename[-1] == "<>":
			filename = '"' + search_file(filename[1:-1].strip(), path) + '"'
		if filename[0] == filename[-1] == '"':
			filename = filename[1:-1].strip()
			try:
				f = open(filename, "r")
			except:
				raise IOError, "error while including file '%s'" % filename
			incLevel += 1
			includes.append((f, curSection))
			line = " "
			while line:
				line = f.readline()
				parse(line)
			f.close()
			curSection = includes[incLevel][1]
			incLevel -= 1
			includes.pop()
		else:
			raise CompileError, "incorrect filename format"
	elif ln[:7].lower() == "#ifndef":
		try:
			define = re.split("\\s+", ln)[1]
		except:
			raise CompileError, "expected const name after '#ifndef'"
		ifsLevel += 1
		ifs.append(define.lower() not in (c[0] for c in constTable))
	elif ln[:6].lower() == "#ifdef":
		try:
			define = re.split("\\s+", ln)[1]
		except:
			raise CompileError, "expected const name after '#ifdef'"
		ifsLevel += 1
		ifs.append(define.lower() in (c[0] for c in constTable))
	else: # command
		buf = []
		if curSection == "header":
			opts = ln.split(",")
			for opt in opts:
				tokens = re.split("\\s+", re.sub("\=\s*\t*", " =", opt), 2)
				if not tokens[0]: del tokens[0]
				counter = 0
				for x in tokens:
					if "=" in x: break
					counter += 1
				cmd = [token.lower() for token in tokens[:counter]]
				try: 	arg = tokens[counter][1:]
				except:	arg = ""
				# check command
				if "name" in cmd:
					header["name"] = arg
				if "library" in cmd:
					header["library"] = {"static": "static" in cmd, "pid": "static" in cmd and getcint(arg) or 0}
		elif curSection == "const":
			const = re.split("\\s+", ln, 1)
			if const[0].lower() not in [c[0] for c in constTable]:
				try:
					constTable.append([const[0].lower(), const[1]])
				except:
					constTable.append([const[0].lower(), "1"])
		elif curSection == "code" or curSection not in reservedSections:
			tokens = splitln(ln)
			tokens[0] = tokens[0].lower()
			if tokens[0] in ("inc", "dec"): #1 - register
 				operand = getop(tokens[1])
 				checkop(operand, [4])
 				buf.append(sectionLength[curSection]) # information for bin writer - command offset
				buf.append(getcmd(tokens[0]))
				buf.append(operand["value"])
				sectionLength[curSection] += 2
			if tokens[0] in ("add", "sub", "mul", "div", "xor", "or", "and", "shl", "shr", "mod"): #1 - register, #2 - any data
 				buf.append(sectionLength[curSection]) # information for bin writer - command offset
				buf.append(getcmd(tokens[0]))
				operand = getop(tokens[1])
				checkop(operand, [4])
				buf.append(operand["value"])
				operand = getop(tokens[2])
				checkop(operand, xrange(5))
				buf.append(operand["flags"])
				buf.append(operand["value"] >> 8)
				buf.append(operand["value"] & 0xff)	
				sectionLength[curSection] += 5
			if tokens[0] in ("mov", "movf"): #1 and #2 - any data
				buf.append(sectionLength[curSection]) # information for bin writer - command offset
				buf.append(getcmd(tokens[0]))
				for x in (1, 2):
					operand = getop(tokens[x])
					checkop(operand, xrange(5))
					buf.append(operand["flags"])
					buf.append(operand["value"] >> 8)
					buf.append(operand["value"] & 0xff)
				sectionLength[curSection] += 7
			if tokens[0] in ("push", "int"): #1 - any data
				buf.append(sectionLength[curSection]) # information for bin writer - command offset
				buf.append(getcmd(tokens[0]))
				operand = getop(tokens[1])
				checkop(operand, xrange(5))
				buf.append(operand["flags"])
				buf.append(operand["value"] >> 8)
				buf.append(operand["value"] & 0xff)
				sectionLength[curSection] += 4
			if tokens[0] == "call": #1 - procedure ptr
				operand = getop(ln[4:])
				checkop(operand, [4, 5])
				buf.append(sectionLength[curSection]) # information for bin writer - command offset
				buf.append(getcmd(tokens[0]))
				buf.append(operand["id"] >> 8)
				buf.append(operand["id"] & 0xff)
				sectionLength[curSection] += 3				
			if tokens[0] == "label":
				labelTable.append({"name": tokens[1], "section": curSection, "offset": sectionLength[curSection]})
			if tokens[0] == "if":
				cond = parsecond(ln[ln.index("(") + 1:ln.index(")")])
				cmd = ln[ln.index(")")+1:].strip()
				buf.append(sectionLength[curSection]) # information for bin writer - command offset
				buf.append(getcmd(tokens[0]))
				buf.append(cond[0])
				buf.append(cond[1]["flags"])
				buf.append(cond[1]["value"] >> 8)
				buf.append(cond[1]["value"] & 0xff)
				buf.append(cond[2]["flags"])
				buf.append(cond[2]["value"] >> 8)
				buf.append(cond[2]["value"] & 0xff)
				sectionLength[curSection] += 8
				sections[curSection].append(buf)
				parse(cmd)
				buf = []
			if tokens[0] == "goto":
				buf.append(sectionLength[curSection])
				buf.append(getcmd(tokens[0]))
				buf.append(tokens[1])
				sectionLength[curSection] += 3
			if tokens[0] in ("ret", "nop"):
				buf.append(sectionLength[curSection])
				buf.append(getcmd(tokens[0]))
				sectionLength[curSection] += 1
			if tokens[0] == "pid":
				buf.append(sectionLength[curSection])
				buf.append(getcmd(tokens[0]))
				operand = getop(tokens[1])
				checkop(operand, [4])
				buf.append(operand["value"])
				operand = getop(tokens[2])
				checkop(operand, [0])
				buf.append(operand["value"] >> 8)
				buf.append(operand["value"] & 0xff)
				sectionLength[curSection] += 4
		elif curSection == "data":
			tokens = re.split("\\s+", ln, 2)
			if tokens[0].lower() == "ends":
				structPtr = -1
				return
			if structPtr != -1:
				vartype = tokens[1].lower()
				varname = tokens[0]
				size = sizeof(vartype)
				if re.match("string", vartype):
					vartype = "string"
				structTable[structPtr]["members"].append((varname, vartype, size))
				return
			vartype = tokens[1].lower()
			varname = tokens[0]
			variable = {"name": varname, "type": vartype, "start": sectionLength["data"], "size": 0}
			if re.match("string", vartype):
				buf, flag = "", 0	
				try: # if string has fixed length
					buf = "\0" * sizeof(vartype)
				except: flag += 1
				try: #if string has initial value
					if len(buf):
						buflen = len(buf)
						buf = getcstr(tokens[2])[:buflen-1] + "\0"
					else:
						buf = getcstr(tokens[2])
				except: flag += 1
				if flag == 2:
					raise TypeError, "can't get length of string '%s'" % varname
				sectionLength["data"] += len(buf)
				variable["size"] = len(buf)
				variable["type"] = "string"
			elif vartype == "int":
				try:	cint = getcint(tokens[2])
				except:	cint = 0
				sectionLength["data"] += 2;
				buf = chr(cint >> 8) + chr(cint & 0xff)
				variable["size"] = 2
			elif vartype == "struct":
				structTable.append({"name": varname, "members": []})
				structPtr = len(structTable)-1
			elif vartype in (struct["name"] for struct in structTable):
				size = sizeof(vartype)
				buf = "\0" * size
				sectionLength["data"] += len(buf)
				variable["sname"] = vartype
				variable["size"] = size
				variable["type"] = "string"
			else:
				raise TypeError, "variable '%s' declared with undefined type '%s'" % (varname, vartype)
			if buf: varTable.append(variable);
		elif curSection == "import":
			cmds = ln.split(",")
			for x in cmds:
				cmd = re.split("\\s+", x, 2)
				if not cmd[0]: del cmd[0]
				if cmd[0].lower() == "from":
					filename = ln[4:].strip()
					if filename[0] == filename[-1] == '"': filename = filename[1:-1]
					# include definitions from file
					try:	o = open(filename, "rb")
					except:
						raise IOError,"can't open file '%s'" % filename
					lib_byte = ord(o.read(1))
					name = o.read(lib_byte & 0x1f)
					curLibrary = name
					for x in xrange(ord(o.read(1))):
						vartype = ord(o.read(1))
						varid = o.read(2)
						varid = (ord(varid[0]) << 8) + ord(varid[1])
						varname = o.read(ord(o.read(1)))
						importTable.append({"library": name, "type": vartype, "id": varid, "name": varname})
				else:
					# search variable in import table
					libname, varname = curLibrary, cmd[0]
					for x in importTable:
						if x["library"] + x["name"] == libname + varname:
							try:
								concat = ""
								for y in cmd: concat += y + " "
								concat = concat.split("as")
								if concat[1]: varname = concat[1].strip()
							except: pass
							if x["type"] == 0:   # int
								varTable.append({"global": True, "type": "int", "name": varname, "start": x["id"]})
							elif x["type"] == 1: # string
								varTable.append({"global": True, "type": "string", "name": varname, "start": x["id"]})
							elif x["type"] == 2: # procedure
								procTable.append({"global": True, "name": varname, "id": x["id"]})
		elif curSection == "export":
			if not header["library"]:
				raise CompileError, "application can't have 'export' section"
			vars = ln.split(",")
			for x in vars:
				var = x.strip().split("as")
				info = varinfo(var[0].strip())
				if info["type"] != "procedure":
					raise CompileError, "only procedures can be exported"
				if len(var) == 2: info["name"] = var[1].strip()
				if not info:
					raise NameError, "undefined variable '%s'" % var[0].strip()
				print info
				exportTable.append(info)
		if not buf: return
		sections[curSection].append(buf)

for arg in argv[1:]:
	if isfile(arg + ".asm"):
		outpath = arg
	elif arg[:2] == "-I":
		path += [abspath(p) for p in arg[2:].split(pathsep)]
if outpath == "":
	raise CompileError, "no input files"

start = time()
f, ln = open(outpath + ".asm", "r"), " "
includes.append((f, ""))
while ln:
	ln = f.readline()
	parse(ln)
f.close()

#write data to file
o = open(basename(outpath) + ".bin", "wb")
#first of all, write header
if header["library"] and not header["name"]:
	raise CompileError, "library must have a name"
if len(header["name"]) > 15:
	raise CompileError, "program name can't be longer then 15 symbols"
if header["library"] and not (0 <= header["library"]["pid"] < 64):
	raise CompileError, "incorrect PID"

o.write(chr(len(header["name"])) + header["name"])
# calucate data size
dataSize = 0
for x in sections["data"]: dataSize += len(x)
# calculate entry point
localProcTable = [x for x in procTable if not x["global"]]
entryPoint = len(header["name"]) + len(localProcTable) * 4 + dataSize + 7
# write function table
o.write(chr(len(localProcTable)))
if header["library"]:
	lib_byte = 0x80 | header["library"]["pid"] & 0x3f
	if header["library"]["static"]: lib_byte |= 0x40
	o.write(chr(lib_byte))
else: o.write("\0")
for x in localProcTable:
	# id
	o.write(chr(x["id"] >> 8) + chr(x["id"] & 0xff))
	# offset
	o.write(chr(entryPoint >> 8) + chr(entryPoint & 0xff))
	entryPoint += sectionLength[x["name"]]

if not len(sections["code"]): entryPoint = 0
o.write(chr(entryPoint >> 8) + chr(entryPoint & 0xff))
o.write(chr(dataSize >> 8) + chr(dataSize & 0xff))
#secondly write DATA section
for x in sections["data"]: o.write(x)
#thirdly write user-defined sections (procedures)
for x in localProcTable:
	curSection = x["name"]
	for y in sections[x["name"]]:
		for z in y[1:]:
			if type(z) == str:
				# is it label?
				try: offset = getlabel(z)
				except: break
				o.write(chr(offset >> 8))
				o.write(chr(offset & 0xff))
			else: o.write(chr(z & 0xff))
#finally write CODE section
for x in sections["code"]:
	for y in x[1:]:
		if type(y) == str:
			# is it label?
			try:
				offset = getlabel(y)
				o.write(chr(offset >> 8))
				o.write(chr(offset & 0xff))
			except: break
		else: o.write(chr(y & 0xff))
o.close()

#generate .INC file
if header["library"] and header["library"]["static"]:
	o = open(basename(outpath) + ".def", "wb")
	lib_byte = header["library"]["static"] * 0x80 | len(header["name"])
	o.write(chr(lib_byte) + header["name"])
	o.write(chr(len(exportTable)))
	for x in exportTable:
		# type
		if x["type"] == "int": 			o.write("\0")
		elif x["type"] == "string":		o.write("\1")
		elif x["type"] == "procedure":	o.write("\2")
		# start
		o.write(chr(x["start"] >> 8) + chr(x["start"] & 0xff))
		# name
		o.write(chr(len(x["name"])) + x["name"])
	o.close()

print "%s.asm: successfully compiled (%i bytes, %f is)" % (outpath, getsize(basename(outpath) + ".bin"), time() - start)
