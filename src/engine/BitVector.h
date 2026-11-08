// Orijinal: BitVector. Duzen: {uint8_t* data; uint32_t numBits; uint32_t numBytes; bool padded}.
#pragma once
#include <stdint.h>
#include <string.h>

class BitVector {
public:
    BitVector(unsigned int numBits, bool padded) : numBits_(numBits), padded_(padded) {
        numBytes_ = numBits >> 3;
        if (numBits & 7) numBytes_ += 1;
        unsigned int alloc = numBytes_;
        if (padded) alloc += (alloc & 3);
        // readBits 32 bit okur; tasmayi onlemek icin 4 bayt pay (orijinalde yok, davranis ayni)
        data_ = new uint8_t[alloc + 4];
        memset(data_, 0, alloc + 4);
    }
    ~BitVector() { delete[] data_; }
    BitVector(const BitVector&) = delete;

    BitVector& operator=(const BitVector& o) {
        if (numBits_ != o.numBits_) {
            numBits_ = o.numBits_;
            numBytes_ = numBits_ >> 3;
            if (numBits_ & 7) numBytes_ += 1;
            delete[] data_;
            data_ = new uint8_t[numBytes_ + 4];
        }
        memset(data_, 0, numBytes_ + 4);
        memcpy(data_, o.data_, numBytes_);
        return *this;
    }

    bool readBit(unsigned int i) const { return (data_[i >> 3] & (1u << (i & 7))) != 0; }
    // En fazla 16 bit: ((word32 >> (i&7)) & ((1<<n)-1)) & 0xffff
    unsigned int readBits(unsigned int i, unsigned int n) const {
        uint32_t w;
        memcpy(&w, data_ + (i >> 3), 4);
        return (w >> (i & 7)) & ((1u << (n & 0xff)) - 1u) & 0xffffu;
    }
    void setAndClearBits(unsigned int i, unsigned int n, unsigned int value) {
        uint32_t w;
        memcpy(&w, data_ + (i >> 3), 4);
        w = (value << (i & 7)) | (w & ~(((1u << (n & 0xff)) - 1u) << (i & 7)));
        memcpy(data_ + (i >> 3), &w, 4);
    }
    void clearAll() { memset(data_, 0, numBytes_); }
    uint8_t* serialize(uint8_t* out) const { memcpy(out, data_, numBytes_); return out + numBytes_; }
    uint8_t* deserialize(uint8_t* in) { memcpy(data_, in, numBytes_); return in + numBytes_; }
    unsigned int numBits() const { return numBits_; }
    unsigned int numBytes() const { return numBytes_; }

private:
    uint8_t* data_;
    unsigned int numBits_, numBytes_;
    bool padded_;
};
