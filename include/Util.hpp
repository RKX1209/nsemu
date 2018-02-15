#ifndef _UTIL_HPP
#define _UTIL_HPP

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#define LINE_BREAK 15

enum RunLevel {
	RUN_LEVEL_RELEASE = 0,
	RUN_LEVEL_DEBUG,
};

static RunLevel curlevel = RUN_LEVEL_DEBUG;

static void util_print(RunLevel level, const char *format, ...) {
	if (curlevel >= level) {
		va_list va;
		va_start (va, format);
		vprintf (format, va);
		va_end (va);
	}
}

#define debug_print(format, ...) util_print (RUN_LEVEL_DEBUG, format, ## __VA_ARGS__)
#define ns_print(format, ...) util_print (RUN_LEVEL_RELEASE, format, ## __VA_ARGS__)
#define ns_abort(format, ...)		\
	ns_print ("%s: ", __func__);	\
	ns_print (format, ## __VA_ARGS__);\
	abort ()

inline void bindump(uint8_t *ptr, size_t size) {
	int i = 0;
	while (i < size) {
		debug_print ("%02x", ptr[i]);
		if ((i + 1) % LINE_BREAK == 0) {
			debug_print ("\n");
		} else {
			debug_print (" ");
		}
		i++;
	}
	if (i % LINE_BREAK != 0) {
		debug_print ("\n");
	}
}

inline uint32_t byte_swap32_uint(uint32_t b) {
        return ((b >> 24) & 0xff) | ((b << 8) & 0xff0000) | ((b >> 8) & 0xff00) | ((b << 24) & 0xff000000);
}

inline uint32_t byte_swap32_str(const char *b) {
	return ((b[3]) << 24) | ((b[2]) << 16) | ((b[1]) << 8) | (b[0]);
}

static inline uint32_t extract32(uint32_t bitfield, int from, int len) {
	assert (from >= 0 && len > 0 && from + len <= 32);
	return (bitfield >> from) & (~0U >> (32 - len));
}

static inline int32_t sextract32(uint64_t bitfield, int from, int len) {
	assert (from >= 0 && len > 0 && from + len <= 32);
	return ((int32_t) (bitfield << (32 - from - len))) >> (32 - len);
}

static inline int64_t sextract64(uint64_t bitfield, int from, int len) {
	assert (from >= 0 && len > 0 && from + len <= 64);
	return ((int64_t) (bitfield << (64 - from - len))) >> (64 - len);
}

static inline int clz32(uint32_t bitfield) {
	return bitfield? __builtin_clz (bitfield): 32;
}

static inline int clz64(uint64_t bitfield) {
	return bitfield? __builtin_clzll (bitfield): 64;
}

/* Return a value with the bottom len bits set (where 0 < len <= 64) */
static inline uint64_t bitmask64(unsigned int length) {
        assert(length > 0 && length <= 64);
        return ~0ULL >> (64 - length);
}

static uint64_t replicate64(uint64_t mask, unsigned int e) {
        assert(e != 0);
        while (e < 64) {
                mask |= mask << e;
                e *= 2;
        }
        return mask;
}
#endif
