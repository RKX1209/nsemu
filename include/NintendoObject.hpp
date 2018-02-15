#ifndef _NINTENDO_OBJECT_HPP
#define _NINTENDO_OBJECT_HPP

class Nsemu;
class NintendoObject {
public:
NintendoObject(std::string path);
~NintendoObject();
virtual int load(Nsemu *nsemu, uint64_t base) = 0;
protected:
std::ifstream fp;
uint32_t length;
};

class Nso : NintendoObject {
public:
Nso(std::string path) : NintendoObject (path) {}
int load(Nsemu *nsemu, uint64_t base);
};

typedef struct {
	uint32_t magic, pad0, pad1, pad2;
	uint32_t textOff, textLoc, textSize, pad3;
	uint32_t rdataOff, rdataLoc, rdataSize, pad4;
	uint32_t dataOff, dataLoc, dataSize;
	uint32_t bssSize;
} NsoHeader;

class Nro : NintendoObject {
public:
Nro(std::string path) : NintendoObject (path) {}
int load(Nsemu *nsemu, uint64_t base);
};

#endif
