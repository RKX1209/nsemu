/* nsemu - LGPL - Copyright 2017 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
RunLevel curlevel;
string read_string(uint8_t *buf, int size) {
        string str((char *)buf, size);
        return str;
}
