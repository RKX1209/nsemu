/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */

#include "Nsemu.hpp"
#include <lz4.h>

NintendoObject::NintendoObject(string path) {
	fp.open(path.c_str(), ios::in | ios::binary);
	fp.seekg(0, ios_base::end);
	length = (uint32_t) fp.tellg();
	fp.seekg(0);
}

NintendoObject::~NintendoObject() {
	fp.close();
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

int Nso::load (Nsemu *nsemu, uint64_t base) {
	NsoHeader hdr;
	fp.read((char *) &hdr, sizeof(NsoHeader));
	if (hdr.magic != host_order32("NSO0"))
		return 0;
	uint32_t size = hdr.dataLoc + hdr.dataSize + hdr.bssSize;
	if (size & 0xfff)
		size = (size & ~0xfff) + 0x1000;
	char *text = decompress(fp, hdr.textOff, hdr.rdataOff - hdr.textOff, hdr.textSize);
	bindump ((uint8_t*)text, 105);
	delete[] text;

	char *rdata = decompress(fp, hdr.rdataOff, hdr.dataOff - hdr.rdataOff, hdr.rdataSize);

	delete[] rdata;

	char *data = decompress(fp, hdr.dataOff, length - hdr.dataOff, hdr.dataSize);

	delete[] data;

	return size;
}

int Nro::load (Nsemu *nsemu, uint64_t base) {

}
