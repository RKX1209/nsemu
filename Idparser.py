import sys, tatsu

grammar = '''
start = { def }+ $ ;

number
	=
	| /0x[0-9a-fA-F]+/
	| /[0-9]+/
	;

def
	=
	| typeDef
	| interface
	;

expression
	=
	| type
	| number
	;

name = /[a-zA-Z_][a-zA-Z0-9_:]*/ ;
sname = /[a-zA-Z_][a-zA-Z0-9_:\-]*/ ;
serviceNameList = @:','.{ sname } ;
template = '<' @:','.{ expression } '>' ;
type = name:name template:[ template ] ;

typeDef = 'type' name:name '=' type:type ';' ;

interface = 'interface' name:name [ 'is' serviceNames:serviceNameList ] '{' functions:{ funcDef }* '}' ;
namedTuple = '(' @:','.{ type [ name ] } ')' ;
namedType = type [ name ] ;
funcDef = '[' cmdId:number ']' name:name inputs:namedTuple [ '->' outputs:( namedType | namedTuple ) ] ';' ;
'''

class Semantics(object):
	def number(self, ast):
		if ast.startswith('0x'):
			return int(ast[2:], 16)
		return int(ast)

	def namedTuple(self, ast):
		return [elem if isinstance(elem, list) else [elem, None] for elem in ast]

	def namedType(self, ast):
		return [ast if isinstance(ast, list) else [ast, None]]

def parseType(type):
	if not isinstance(type, tatsu.ast.AST) or 'template' not in type:
		return type
	name, template = type['name'], type['template']
	if template is None:
		return [name]
	else:
		return [name] + map(parseType, template)

def parse(data):
	ast = tatsu.parse(grammar, data, semantics=Semantics(), eol_comments_re=r'\/\/.*?$')

	types = {}
	for elem in ast:
		if 'type' not in elem:
			continue
		#assert elem['name'] not in types
		types[elem['name']] = parseType(elem['type'])

	ifaces = {}
	services = {}
	for elem in ast:
		if 'functions' not in elem:
			continue
		#assert elem['name'] not in ifaces
		ifaces[elem['name']] = iface = {}
		if elem['serviceNames']:
			services[elem['name']] = list(elem['serviceNames'])

		for func in elem['functions']:
			if func['name'] in iface:
				print >>sys.stderr, 'Duplicate function %s in %s' % (func['name'], elem['name'])
				sys.exit(1)

			assert func['name'] not in iface
			iface[func['name']] = fdef = {}
			fdef['cmdId'] = func['cmdId']
			fdef['inputs'] = [(name, parseType(type)) for type, name in func['inputs']]
			if func['outputs'] is None:
				fdef['outputs'] = []
			elif isinstance(func['outputs'], tatsu.ast.AST):
				fdef['outputs'] = [(None, parseType(func['outputs']))]
			else:
				fdef['outputs'] = [(name, parseType(type)) for type, name in func['outputs']]

	return types, ifaces, services
