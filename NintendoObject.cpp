/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */

#include "Nsemu.hpp"
#include <lz4.h>

NintendoObject::NintendoObject(string path) {
	fp.open (path.c_str (), ios::in | ios::binary);
	fp.seekg (0, ios_base::end);
	length = (uint32_t) fp.tellg ();
	fp.seekg (0);
}

NintendoObject::~NintendoObject() {
	fp.close ();
}

char *decompress(ifstream &fp, uint32_t offset, uint32_t csize, uint32_t usize) {
	fp.seekg (offset);
	char *buf = new char[csize];
	char *obuf = new char[usize];
	fp.read (buf, csize);
	assert (LZ4_decompress_safe (buf, obuf, csize, usize) == usize);
	delete[] buf;
	return obuf;
}

int Nso::load(Nsemu *nsemu) {
	NsoHeader hdr;
	fp.read ((char *) &hdr, sizeof(NsoHeader));
	if (hdr.magic != byte_swap32_str ("NSO0")) {
		return 0;
	}
	uint32_t size = hdr.dataLoc + hdr.dataSize + hdr.bssSize;
	if (size & 0xfff) {
		size = (size & ~0xfff) + 0x1000;
	}

	char *text = decompress (fp, hdr.textOff, hdr.rdataOff - hdr.textOff, hdr.textSize);
	if (!Memory::CopytoEmuByName (nsemu, (void *) text, ".text", hdr.textSize)) {
		delete[] text;
		ns_abort ("Failed to copy to .text\n");
	}
	/* --- For test --- */
	uint8_t *txt_dump = new uint8_t[hdr.textSize];
	Memory::CopyfromEmuByName (nsemu, (void *) txt_dump, ".text", hdr.textSize);
	bindump (txt_dump, 105);
	/* ---------------- */
	delete[] text;

	char *rdata = decompress (fp, hdr.rdataOff, hdr.dataOff - hdr.rdataOff, hdr.rdataSize);
	if (!Memory::CopytoEmuByName (nsemu, (void *) rdata, ".rdata", hdr.rdataSize)) {
		delete[] rdata;
		ns_abort ("Failed to copy to .rdata\n");
	}
	delete[] rdata;

	char *data = decompress (fp, hdr.dataOff, length - hdr.dataOff, hdr.dataSize);
	if (!Memory::CopytoEmuByName (nsemu, (void *) data, ".data", hdr.dataSize)) {
		delete[] data;
		ns_abort ("Failed to copy to .data\n");
	}
	delete[] data;

	return size;
}

int Nro::load(Nsemu *nsemu) {
	return 0;
}
