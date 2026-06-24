#ifndef UTF8_SANITIZER_H
#define UTF8_SANITIZER_H

#include <stdint.h>
#include <stddef.h>

void sanitize_utf8(uint8_t* input_buffer, size_t len, uint8_t subst);

#endif
