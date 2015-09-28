#ifndef DECODE_LOOKUP_CLS_HPP
#define DECODE_LOOKUP_CLS_HPP

namespace http {

static inline int
lookup_cls (uint32_t const tbl[], uint32_t const octet)
{
    uint32_t const i = octet >> 3;
    uint32_t const count = (7 - (octet & 7)) << 2;
    return octet < 128 ? ((tbl[i] >> count) & 0x0f) : 0;
}

}//namespace http

#endif
