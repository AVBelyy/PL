#!/usr/bin/python
# -*- coding: utf-8 

#
#	(C) Anton Belyy, 2011
#

import re
from sys import argv, exit
from time import time
from os.path import getsize
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
	("mod",		0x16)  #
)

curLine, curSection, sectionLength, sections = 0, "", {"code": 0, "data": 0}, {"code": [], "data": []}
reservedSections = ("header", "data", "const", "import", "export", "code")
header = {"name": "", "library": False}
varTable, constTable, labelTable, procTable = [], [], [], []
exportTable, importTable = [], []
constStrCounter = 0
outpath = ""

def error(errtype, msg, solution):
	print "Файл \"%s\", строка %i, %s \"%s\"\n  %s: %s\n  Возможное решение: %s" % (outpath and (outpath + ".asm") or "None", curLine, curSection in reservedSections and "секция" or "процедура", curSection, errtype, msg, solution)
	exit(1)

try:	outpath = argv[1]
except:	error("Ошибка компиляции", "не указано имя файла для компиляции", "укажите в командной строке имя файла, который нужно\n  скомпилировать")

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
	if cint[0] == cint[-1] == "'":	return ord(encode(cint[1:-1])[0])
	if cint[:2].lower() == "0x":	return int(cint, 16) & 0x7fff
	elif cint[:2].lower() == "0b":	return int(cint, 2) & 0x7fff
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
		solution = "попробуйте использовать операнд следующего типа:"
		for x in flags:
			if x == 0: solution += "\n   * целочисленная константа (const int)"
			if x == 1: solution += "\n   * символ (char)"
			if x == 2: solution += "\n   * целое число (int)"
			if x == 3: solution += "\n   * указатель на регистр (register*)"
 			if x == 4: solution += "\n   * регистр (register)"
			if x == 5: solution += "\n   * указатель на процедуру (void(*)())"
		error("Ошибка типов", "неверный тип операнда", solution)

def getop(op):
	operand = {"flags": 0, "value": 0}
	op = op.strip()
	for x in constTable:
		if x[0] == op: return getop(x[1])
	try:
		try:
			args = splitln(op[op.index("("):].strip()[1:-1])
			for x in args:
				if x.strip(): parse("PUSH " + x)
			op = op[:op.index("(")].strip()
			operand["flags"] = 4
		except: pass
		if op in [proc["name"] for proc in procTable]:
			return {"flags": operand["flags"] or 5, "value": 0, "id": procTable[procid(op)]["id"]}
	except: pass
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
		if op[0] == "&": op, operand["flags"] = op[1:], 1
		if len(op) == 2 and op[0].lower() == "r" and ord(op[1]) in xrange(48, 58):
			return {"flags": (not operand["flags"]) + 3, "value": int(op[1])}
		for x in varTable:
			if x["name"] == op:
				if x["type"] == "int" and operand["flags"]: operand["flags"] = 2
				operand["value"] = x["start"]
				return operand
		if op[:2] not in ("0x", "0b"):
			error("Ошибка именования", "неизвестная переменная '%s'" % op.strip(), "исправьте имя переменной в программе")
	operand["value"] = getcint(op)
	return operand

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
		if fx == 0: return "\0"
		if fx["name"] == "fill":
			for x in xrange(getcint(fx["args"][0])): out += chr(symbol)
	return out

def parse(ln):
	global curSection
	ln = ln.strip()
	if not ln: return
	if ln[-1] == ":": # label
		curSection = ln[:-1].strip().lower()
		if curSection not in reservedSections: # is procedure
			if curSection in [proc["name"] for proc in procTable]: # already defined
				error("Ошибка именования", "процедура уже описана в этом модуле", "исключите одно из описаний процедуры")
			procTable.append({"global": False, "name": curSection, "id": len(procTable)})
		try:
			if sections[curSection]: pass
		except:
			sections[curSection] = []
			sectionLength[curSection] = 0		
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
			constTable.append(re.split("\\s+", ln, 1))
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
		elif curSection == "data":
			tokens = re.split("\\s+", ln, 2)
			vartype = tokens[0].lower()
			varname = tokens[1]
			variable = {"name": varname, "type": vartype, "start": sectionLength["data"], "size": 0}
			if vartype == "string":
				try:
					buf = getcstr(tokens[2])
					sectionLength["data"] += len(buf)
					variable["size"] = len(buf)
				except: pass
			elif vartype == "int":
				try:	cint = getcint(tokens[2])
				except:	cint = 0
				sectionLength["data"] += 2;
				buf = chr(cint >> 8) + chr(cint & 0xff)
				variable["size"] = 2
			varTable.append(variable);
		elif curSection == "import":
			cmds = ln.split(",")
			for x in cmds:
				cmd = re.split("\\s+", x, 2)
				if not cmd[0]: del cmd[0]
				if cmd[0].lower() == "include":
					filename = ln[7:].strip()
					if filename[0] == filename[-1] == '"': filename = filename[1:-1]
					# include definitions from file
					try:	o = open(filename, "rb")
					except:
						error("Ошибка ввода\вывода", "невозможно открыть файл '%s'" % filename, "проверьте корректность имени файла")
					lib_byte = ord(o.read(1))
					name = o.read(lib_byte & 0x1f)
					for x in xrange(ord(o.read(1))):
						vartype = ord(o.read(1))
						varid = o.read(2)
						varid = (ord(varid[0]) << 8) + ord(varid[1])
						varname = o.read(ord(o.read(1)))
						importTable.append({"library": name, "type": vartype, "id": varid, "name": varname})
				else:
					# search variable in import table
					libname, varname = cmd[0].split("::")
					for x in importTable:
						if x["library"] + x["name"] == libname + varname:
							try:
								concat = ""
								for y in cmd: concat += y + " "
								concat = concat.split("=>")
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
				error("Ошибка компиляции", "приложение не может иметь EXPORT-секцию", "перенесите глобальные переменные вашей программы\n                     во внешнюю библиотеку или объявите их локальными")
			vars = ln.split(",")
			for x in vars:
				var = x.strip().split("=>")
				info = varinfo(var[0].strip())
				if len(var) == 2: info["name"] = var[1].strip()
				if not info:
					error("Ошибка именования", "неизвестная переменная '%s'" % var[0].strip(), "исправьте имя переменной в программе")
				exportTable.append(info)
		if not buf: return
		sections[curSection].append(buf)

start = time()
f, ln = open("apps/" + outpath + ".asm", "r"), " "
while ln:
	ln = f.readline()
	curLine += 1
	parse(ln)
f.close()

#write data to file
o = open(outpath + ".bin", "wb")
#first of all, write header
if header["library"] and not header["name"]:
	error("Ошибка компиляции", "библиотека должна иметь имя", "укажите имя библиотеки в секции HEADER")
if len(header["name"]) > 15:
	error("Ошибка компиляции", "имя программы превышает 15 символов", "исправьте имя программы")
if header["library"] and not (0 <= header["library"]["pid"] < 64):
	error("Ошибка компиляции", "PID статической библиотеки имеет неверное значение", "проверьте, лежит ли PID в пределах от 0 до 63")

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
if header["library"]:
	o = open(outpath + ".inc", "wb")
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

print "%s.asm: успешно скомпилировано (%i байт, %f с)" % (outpath, getsize(outpath + ".bin"), time() - start)
exit(0)
