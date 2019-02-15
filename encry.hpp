#ifndef __ENCRY_HPP__
#define __ENCRY_HPP__

static inline char * XRO(char *buf, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        buf[i] ^= 1;
    }
}

static inline void Decrypt(char *buf, size_t len)
{
    XRO(buf, len);
}

static inline void Encry(char *buf, size_t len)
{
    XRO(buf, len);
}


#endif //__ENCRY_HPP__
