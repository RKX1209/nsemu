import glob, hashlib, json, os, os.path, re, sys
from pprint import pprint
import Idparser, Partialparser
from cStringIO import StringIO

def emitInt(x):
	return '0x%x' % x if x > 9 else str(x)

typemap = dict(
	i8='int8_t',
	i16='int16_t',
	i32='int32_t',
	i64='int64_t',
	i128='int128_t',
	u8='uint8_t',
	u16='uint16_t',
	u32='uint32_t',
	u64='uint64_t',
	u128='uint128_t',
	f32='float',
	pid='uint64_t',
	bool='bool',
)

typesizes = dict(
	i8=1,
	i16=2,
	i32=4,
	i64=8,
	i128=16,
	u8=1,
	u16=2,
	u32=4,
	u64=8,
	u128=16,
	f32=4,
	pid=8,
	bool=1,
)

allTypes = None

def typeSize(type):
	if type[0] in ('unknown', 'i8', 'u8'):
		return 1
	elif type[0] == 'bytes':
		return type[1]
	elif type[0] in allTypes:
		return typeSize(allTypes[type[0]])
	elif type[0] in typesizes:
		return typesizes[type[0]]
	return 1

def splitByNs(obj):
	ons = {}
	for type, x in obj.items():
		ns = type.rsplit('::', 1)[0] if '::' in type else None
		name = type.rsplit('::', 1)[-1]
		if ns not in ons:
			ons[ns] = {}
		ons[ns][name] = x
	return ons

def retype(spec, noIndex=False):
	if spec[0] == 'unknown':
		return 'uint8_t'
	elif spec[0] == 'bytes':
		return 'uint8_t%s' % ('[%s]' % emitInt(spec[1]) if not noIndex else ' *')
	else:
		return typemap[spec[0]] if spec[0] in typemap else spec[0];

def formatParam(param, input, i):
	name, spec = param
	if name is None:
		name = '_%i' % i

	hasSize = False

	if spec[0] == 'align':
		return formatParam((name, spec[2]), input, i)

	if spec[0] == 'bytes':
		type = 'uint8_t *'
	elif spec[0] == 'unknown':
		assert False
	elif spec[0] == 'buffer':
		type = '%s *' % retype(spec[1])
		hasSize = True
	elif spec[0] == 'array':
		type = retype(spec[1]) + ' *'
		hasSize = True
	elif spec[0] == 'object':
		type = '%s*' % spec[1][0]
	elif spec[0] == 'IpcService':
		type = 'IpcService*'
	else:
		type = typemap[spec[0]] if spec[0] in typemap else spec[0]

	if type.endswith(']'):
		arrspec = type[type.index('['):]
		type = type[:-len(arrspec)]
	else:
		arrspec = ''

	return '%s%s %s%s%s' % (type, '&' if not input and (not type.endswith('*') and not arrspec) else '', name, arrspec, ', unsigned int %s_size' % name if hasSize else '')

def generatePrototype(func):
	return ', '.join([formatParam(x, True, i) for i, x in enumerate(func['inputs'])] + [formatParam(x, False, i + len(func['inputs'])) for i, x in enumerate(func['outputs'])])

def isPointerType(type):
	if type[0] in typesizes:
		return False
	elif type[0] == 'bytes':
		return True
	elif type[0] in allTypes:
		return isPointerType(allTypes[type[0]])
	return True

INIT = 'INIT'
AFTER = 'AFTER'

def generateCaller(qname, fname, func):
	def tempname():
		tempI[0] += 1
		return 'temp%i' % tempI[0]
	params = []
	logFmt, logElems = [], []
	tempI = [0]
	inpOffset = 8
	bufOffs = {}
	hndOff = 0
	objOff = 0
	bufSizes = 0
	for name, elem in func['inputs']:
		type, rest = elem[0], elem[1:]
		if type in ('array', 'buffer'):
			if rest[1] not in bufOffs:
				bufOffs[rest[1]] = 0
			cbo = bufOffs[rest[1]]
			bufOffs[rest[1]] += 1
			an, sn, bn = tempname(), tempname(), tempname()
			yield 'unsigned int %s;' % sn
			yield 'auto %s = req->GetBuffer(%s, %i, %s);' % (an, emitInt(rest[1]), cbo, sn)
			yield 'auto %s = new uint8_t[%s];' % (bn, sn)
			yield 'ARMv8::ReadBytes(%s, %s, %s);' % (an, bn, sn)
			params.append('(%s *) %s' % (retype(rest[0]), bn))
			params.append(sn)
			logFmt.append('%s *%s= buffer<0x%%lx>' % (retype(rest[0]), '%s ' % name if name else ''))
			logElems.append(sn)
			bufSizes += 1
			yield AFTER, 'delete[] %s;' % bn
		elif type == 'object':
			params.append('IPC::GetHandle<%s*>(req->GetMoved(%i))' % (rest[0][0], objOff))
			logFmt.append('%s %s= 0x%%x' % (rest[0][0], '%s ' % name if name else ''))
			logElems.append('req->GetMoved(%i)' % objOff)
			objOff += 1
		elif type == 'IpcService':
			params.append('IPC::GetHandle<IpcService*>(req->GetCopied(%i))' % (objOff))
			logFmt.append('KObject %s= 0x%%x' % ('%s ' % name if name else ''))
			logElems.append('req->GetCopied(%i)' % hndOff)
			hndOff += 1
		elif type == 'pid':
			params.append('req->pid')
		else:
			if elem[0] == 'align':
				alignment = elem[1]
				elem = elem[2]
			else:
				alignment = min(8, typeSize(elem))
			while inpOffset % alignment:
				inpOffset += 1
			if isPointerType(elem):
				params.append('req->GetDataPointer<%s>(%s)' % (retype(elem, noIndex=True), emitInt(inpOffset)))
				logFmt.append('%s %s= %%s' % (retype(elem), '%s ' % name if name else ''))
				logElems.append('read_string(req->GetDataPointer<uint8_t *>(%s), %s).c_str()' % (emitInt(inpOffset), emitInt(typeSize(elem))))
			else:
				params.append('req->GetData<%s>(%s)' % (retype(elem), emitInt(inpOffset)))
				if typeSize(elem) == 16:
					logFmt.append('%s %s= %%s' % (retype(elem), '%s ' % name if name else ''))
					logElems.append('read_string(req->GetDataPointer<uint8_t *>(%s), %s).c_str()' % (emitInt(inpOffset), emitInt(typeSize(elem))))
				else:
					type = retype(elem)
					ct = '0x%x'
					if type == 'float':
						ct = '%f'
					elif typeSize(elem) == 8:
						ct = '0x%%lx'
					logFmt.append('%s %s= %s' % (type, '%s ' % name if name else '', ct))
					logElems.append('%sreq->GetData<%s>(%s)' % ('(double) ' if type == 'float' else '', type, emitInt(inpOffset)))
			inpOffset += typeSize(elem)

	outOffset = 8
	hndOff = 0
	objOff = 0
	for _, elem in func['outputs']:
		type, rest = elem[0], elem[1:]
		if type in ('array', 'buffer'):
			if rest[1] not in bufOffs:
				bufOffs[rest[1]] = 0
			cbo = bufOffs[rest[1]]
			bufOffs[rest[1]] += 1
			an, sn, bn = tempname(), tempname(), tempname()
			yield 'unsigned int %s;' % sn
			yield 'auto %s = req->GetBuffer(%s, %i, %s);' % (an, emitInt(rest[1]), cbo, sn)
			yield 'auto %s = new uint8_t[%s];' % (bn, sn)
			params.append('(%s *) %s' % (retype(rest[0]), bn))
			params.append(sn)
			bufSizes += 1
			yield AFTER, 'ARMv8::WriteBytes(%s, %s, %s);' % (an, bn, sn)
			yield AFTER, 'delete[] %s;' % bn
		elif type == 'object':
			tn = tempname()
			yield '%s* %s;' % (rest[0][0], tn)
			params.append(tn)
			yield AFTER, 'if(%s != nullptr)' % tn
			yield AFTER, '\tresp->SetMove(%i, IPC::NewHandle((IpcService *)%s));' % (objOff, tn)
			objOff += 1
		elif type == 'IpcService':
			tn = tempname()
			yield 'IpcService *%s;' % tn
			params.append(tn)
			yield AFTER, 'if(%s != nullptr)' % tn
			yield AFTER, '\tresp->SetCopy(%i, IPC::NewHandle(%s));' % (hndOff, tn)
			hndOff += 1
		elif type == 'pid':
			assert False
		else:
			if elem[0] == 'align':
				alignment = elem[1]
				elem = elem[2]
			else:
				alignment = min(8, typeSize(elem))
			while outOffset % alignment:
				outOffset += 1
			if isPointerType(elem):
				tn = tempname()
				yield 'auto %s = resp->GetDataPointer<%s>(%s);' % (tn, retype(elem, noIndex=True), emitInt(outOffset))
				params.append(tn)
			else:
				params.append('*resp->GetDataPointer<%s *>(%s)' % (retype(elem), emitInt(outOffset)))
			outOffset += typeSize(elem)

	if len(func['outputs']) + len(func['inputs']) + bufSizes != len(params):
		yield 'return 0xf601;'
		return

	yield INIT, 'resp->GenBuf(%i, %i, %i);' % (objOff, hndOff, outOffset - 8)
	if len(logFmt):
		yield 'ns_print("IPC message to %s: %s\\n"%s);' % (qname + '::' + fname, ', '.join(logFmt), (', ' + ', '.join(logElems)) if logElems else '')
	else:
		yield 'ns_print("IPC message to %s\\n");' % (qname + '::' + fname)
	yield 'resp->error_code = %s(%s);' % (fname, ', '.join(params))
	yield AFTER
	yield 'return 0;'

def reorder(gen):
	after = []
	before = []
	for x in gen:
		if x == AFTER:
			for elem in after:
				yield elem
		elif isinstance(x, tuple) and x[0] == INIT:
			yield x[1]
			for elem in before:
				yield elem
			before = None
		elif isinstance(x, tuple) and x[0] == AFTER:
			after.append(x[1])
		elif before is not None:
			before.append(x)
		else:
			yield x

def parsePartials(code):
	code = '\n'.join(re.findall(r'/\*\$IPC\$(.*?)\*/', code, re.M|re.S))
	return Partialparser.parse(code)

usedInts = []
def uniqInt(*args):
	args = ''.join(map(str, args))
	i = int(hashlib.md5(args).hexdigest()[:8], 16)
	while True:
		if i not in usedInts:
			usedInts.append(i)
			return i
		i += 1

def main():
	global allTypes

	fns = ['Ipcdefs/Auto.def'] + [x for x in glob.glob('Ipcdefs/*.def') if x != 'Ipcdefs/Auto.def']

	if os.path.exists('Ipcdefs/cache') and all(os.path.getmtime('Ipcdefs/cache') > os.path.getmtime(x) for x in fns):
		res = json.load(file('Ipcdefs/cache'))
	else:
		res = Idparser.parse('\n'.join(file(fn).read() for fn in fns))
		with file('Ipcdefs/cache', 'w') as fp:
			json.dump(res, fp)
	types, ifaces, services = res

	allTypes = types

	typesByNs = splitByNs(types)
	ifacesByNs = splitByNs(ifaces)

	#print typesByNs, ifacesByNs

	namespaces = {x : [] for x in typesByNs.keys() + ifacesByNs.keys()}

	for ns, types in typesByNs.items():
		for name, spec in sorted(types.items(), key=lambda x: x[0]):
			retyped, plain = retype(spec, noIndex=True), retype(spec)
			namespaces[ns].append('using %s = %s;%s' % (name, retyped, ' // ' + plain if retyped != plain else ''))

	for ns, ifaces in ifacesByNs.items():
		for name in sorted(ifaces.keys()):
			namespaces[ns].append('class %s;' % name)

	fp = StringIO()
	print >>fp, '#ifndef __IPCSTUBS_HPP__'
	print >>fp, '#define __IPCSTUBS_HPP__'
    	print >>fp
	impled = (u"nn::fssrv::sf::IFileSystemProxy", u"hoge")
    	# XXX:
    	tmp_srvs = {k: services[k] for k in impled if k in services}
	print tmp_srvs
	print >>fp, '#define SERVICE_MAPPING() do { \\'
	for iname, snames in sorted(tmp_srvs.items(), key=lambda x: x[0]):
	 	for sname in snames:
	 		print >>fp, '\tSERVICE("%s", %s); \\' % (sname, iname)
	print >>fp, '} while(0)'
	print >>fp

	for ns, elems in sorted(namespaces.items(), key=lambda x: x[0]):
		if ns is not None:
			print >>fp, 'namespace %s {' % ns
		hasUsing = False
		for elem in elems:
			if not hasUsing and elem.startswith('using'):
				hasUsing = True
			elif hasUsing and elem.startswith('class'):
				print >>fp
				hasUsing = False
			print >>fp, ('\t' if ns is not None else '') + elem
		if ns is not None:
			print >>fp, '}'

	print >>fp

	allcode = '\n'.join(file(fn, 'r').read() for fn in glob.glob('Service/*.cpp'))

	partials = parsePartials(allcode)

	for ns, ifaces in sorted(ifacesByNs.items(), key=lambda x: x[0]):
		print >>fp, '%snamespace %s {' % ('//// ' if ns is None else '', ns)
		for name, funcs in sorted(ifaces.items(), key=lambda x: x[0]):
			qname = '%s::%s' % (ns, name) if ns else name
			partial = partials[qname] if qname in partials else None
			print >>fp, '\tclass %s : public IpcService {' % name
			print >>fp, '\tpublic:'
			if re.search('(^|[^a-zA-Z0-9:])%s::%s[^a-zA-Z0-9:]' % (qname, name), allcode):
				print >>fp, '\t\t%s();' % name
			else:
				print >>fp, '\t\t%s() {}' % name
			if re.search('(^|[^a-zA-Z0-9:])%s::~%s[^a-zA-Z0-9:]' % (qname, name), allcode):
				print >>fp, '\t\t~%s();' % name
			print >>fp, '\t\tuint32_t Dispatch(IpcMessage *req, IpcMessage *resp) {'
			print >>fp, '\t\t\tswitch(req->cmd_id) {'
			for fname, func in sorted(funcs.items(), key=lambda x: x[1]['cmdId']):
				print >>fp, '\t\t\tcase %i: {' % func['cmdId'];
				print >>fp, '\n'.join('\t\t\t\t' + x for x in reorder(generateCaller(qname, fname, func)))
				print >>fp, '\t\t\t}'
			print >>fp, '\t\t\tdefault:'
			print >>fp, '\t\t\t\tns_abort("Unknown message cmdId %%u to interface %s", req->cmd_id);' % ('%s::%s' % (ns, name) if ns else name)
			print >>fp, '\t\t\t}'
			print >>fp, '\t\t}'
			for fname, func in sorted(funcs.items(), key=lambda x: x[0]):
				implemented = re.search('[^a-zA-Z0-9:]%s::%s[^a-zA-Z0-9:]' % (qname, fname), allcode)
				print >>fp, '\t\tuint32_t %s(%s);' % (fname, generatePrototype(func))
			if partial:
				for x in partial[0]:
					print >>fp, '\t\t%s' % x
			print >>fp, '\t};'
		print >>fp, '%s}' % ('//// ' if ns is None else '')

		print >>fp, '#ifdef DEFINE_STUBS'
		for name, funcs in sorted(ifaces.items(), key=lambda x: x[0]):
			qname = '%s::%s' % (ns, name) if ns else name
			partial = partials[qname] if qname in partials else None
			for fname, func in sorted(funcs.items(), key=lambda x: x[0]):
				implemented = re.search('[^a-zA-Z0-9:]%s::%s[^a-zA-Z0-9:]' % (qname, fname), allcode)
				if not implemented:
					print >>fp, 'uint32_t %s::%s(%s) {' % (qname, fname, generatePrototype(func))
					print >>fp, '\tns_print("Stub implementation for %s::%s\\n");' % (qname, fname)
					for i, (name, elem) in enumerate(func['outputs']):
						if elem[0] == 'object' and elem[1][0] != 'IUnknown':
							# name = name if name else '_%i' % (len(func['inputs']) + i)
							# print >>fp, '\t%s = buildInterface(%s);' % (name, elem[1][0])
							if elem[1][0] in partials and partials[elem[1][0]][1]:
								print 'Bare construction of interface %s requiring parameters.  Created in %s::%s for parameter %s' % (elem[1][0], qname, fname, name)
								sys.exit(1)
						# elif elem[0] == 'KObject':
						# 	# name = name if name else '_%i' % (len(func['inputs']) + i)
						# 	print >>fp, '\t%s = make_shared<FauxHandle>(0x%x);' % (name, uniqInt(qname, fname, name))
					print >>fp, '\treturn 0;'
					print >>fp, '}'
		print >>fp, '#endif // DEFINE_STUBS'

	print >>fp, '#endif // __IPCSTUBS_HPP__'
	code = fp.getvalue()
	if os.path.exists('include/IpcStubs.hpp'):
		with file('include/IpcStubs.hpp', 'r') as fp:
			match = fp.read() == code
	else:
		match = False
	if not match:
		with file('include/IpcStubs.hpp', 'w') as fp:
			fp.write(code)

if __name__=='__main__':
	main(*sys.argv[1:])
