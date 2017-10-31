#include "Nsemu.hpp"
#include <lz4.h>

NintendoObject::NintendoObject(string path) {
	fp.open(path, ios::in | ios::binary);
	fp.seekg(0, ios_base::end);
	length = (uint32_t) fp.tellg();
	fp.seekg(0);
}

char *decompress(ifstream &fp, uint32_t offset, uint32_t csize, uint32_t usize) {
	fp.seekg(offset);
	char *buf = new char[csize];
	char *obuf = new char[usize];
	fp.read(buf, csize);
	assert(LZ4_decompress_safe(buf, obuf, csize, usize) == usize);
	delete[] buf;
	return obuf;
}

int Nso::load (Nsemu *nsemu, uint64_t *base) {

}

int Nro::load (Nsemu *nsemu, uint64_t *base) {
  
}
