import re

def parse(data):
	partials = {}
	for name, body in re.findall('^\s*partial (.*?)\s*{(.*?)}', data, re.M|re.S):
		if name not in partials:
			partials[name] = [], []
		members, params = partials[name]
		for elem in body.split(';'):
			elem = elem.strip()
			if not elem:
				continue
			if elem.startswith('[ctor]'):
				elem = elem[6:].strip()
				type, name = re.match('^(.*?)([_a-zA-Z][_a-zA-Z0-9]+)$', elem).groups()
				params.append((type, name))

			members.append(elem + ';')

	return partials
