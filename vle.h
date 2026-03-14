#ifndef VLE_H__
#define VLE_H__

// Variable length encoding (un-)marshaling.
// See README.md for the format description.

#if __cplusplus < 201103L
#error "This header requires at least C++11"
#endif

#include <cstdint>
#include <sys/types.h>
#include <type_traits>
#include <vector>

namespace vle {

static constexpr ssize_t INCOMPLETE = -1;  // input too short to contain a complete encoded value
static constexpr ssize_t DUMMY = -2;       // the first bytes represent the dummy value

template<typename IT> struct Result {  // return type of the functions that decode byte sequences
  IT n;        // decoded value (meaningful only when len > 0)
  ssize_t len; // number of bytes consumed on success (> 0), or INCOMPLETE/DUMMY on error
};

namespace vle_private {  // private stuff, not part of the public api

template<typename IT, class BVec=std::vector<std::uint8_t>> std::size_t inline Encode(IT n, const IT stop, std::uint8_t signBit, BVec* v) {
  std::size_t size = 1;
  IT b = n & 0x7f;
  while (n >= stop) {
    v->push_back(b);
    n >>= 7;
    b = (std::uint8_t) (n & 0x7f) | 0x80;
    ++size;
  }
  v->push_back(b | signBit);
  auto vend = v->end();
  auto vstart = vend - size;
  --vend;
  while (vstart < vend) {
    const std::uint8_t x = *vstart;
    *vstart++ = *vend;
    *vend-- = x;
  }
  return size;
}

// decodes bytes between start and end iterators, returning the decoded value and byte count
template<typename IT, typename BIter> inline Result<IT> Decode(BIter start, BIter end, IT n) {
  BIter p = start;
  while ((*p & 0x80) != 0) {
    if (++p >= end) return Result<IT>{IT{}, INCOMPLETE};
    n = (n << 7) | (IT) (*p & 0x7f);
  }
  return Result<IT>{n, (ssize_t)(p - start + 1)};
}

}  // namespace vle_private

// the representation of a dummy unsigned integer is a single byte equal to DUMMY_UNSIGNED
static constexpr std::uint8_t DUMMY_UNSIGNED = 0x80;

// appends the representation of the dummy signed value to the bytes container v
// the container must have a push_back() method
// returns the number of bytes appended (2)
template<class BVec=std::vector<std::uint8_t>> inline std::size_t EncodeDummySigned(BVec* v) {
  v->push_back(0x80);
  v->push_back(0x00);
  return 2;
}

// appends the representation of the dummy unsigned value to the bytes container v
// the container must have a push_back() method
// returns the number of bytes appended (1)
// it's equivalent to this expression:   v->push_back(DUMMY_UNSIGNED), 1
template<class BVec=std::vector<std::uint8_t>> inline std::size_t EncodeDummyUnsigned(BVec* v) {
  v->push_back(DUMMY_UNSIGNED);
  return 1;
}

// appends the encoded representation of a signed integer n to the bytes container v
// the container must have end() and push_back() methods
// returns the number of appended bytes
template<typename IT, class BVec=std::vector<std::uint8_t>> inline std::size_t EncodeSigned(IT n, BVec* v) {
  if (n >= 0) {
    return vle_private::Encode(n, (IT) 0x40, 0, v);
  } else {
    return vle_private::Encode((IT) (-n - 1), (IT) 0x40, 0x40, v);
  }
}

// appends the encoded representation of an unsigned integer n to the bytes container v
// the container must have end() and push_back() methods
// returns the number of appended bytes
template<typename IT, class BVec=std::vector<std::uint8_t>> inline std::size_t EncodeUnsigned(IT n, BVec* v) {
  return vle_private::Encode(n, (IT) 0x80, 0, v);
}

// decodes the sequence of bytes between two iterators as a signed integer
// Result.len is the number of bytes consumed (> 0), INCOMPLETE, or DUMMY
template<typename IT, typename BIter> inline Result<IT> DecodeSigned(BIter start, BIter end) {
  if (start >= end) return Result<IT>{IT{}, INCOMPLETE};
  if (*start == 0x80) {
    const BIter next = start + 1;
    if (next >= end) return Result<IT>{IT{}, INCOMPLETE};
    if (*next == 0x00) return Result<IT>{IT{}, DUMMY};
  }
  if ((*start & 0x40) == 0) {
    return vle_private::Decode(start, end, (IT) (*start & 0x3f));
  } else {
    Result<IT> dr = vle_private::Decode(start, end, (IT) (*start & 0x3f));
    dr.n = -dr.n - 1;
    return dr;
  }
}

// decodes the sequence of bytes between two iterators as an unsigned integer
// Result.len is the number of bytes consumed (> 0), INCOMPLETE, or DUMMY
template<typename IT, typename BIter> inline Result<IT> DecodeUnsigned(BIter start, BIter end) {
  if (start >= end) return Result<IT>{IT{}, INCOMPLETE};
  if (*start != DUMMY_UNSIGNED) return vle_private::Decode(start, end, (IT) (*start & 0x7f));
  return Result<IT>{IT{}, DUMMY};
}

// appends the encoded representation of an integer n to the bytes container v
// automatically dispatches to EncodeSigned or EncodeUnsigned based on the signedness of IT
// the container must have end() and push_back() methods
// returns the number of appended bytes
template<typename IT, class BVec=std::vector<std::uint8_t>>
inline typename std::enable_if<std::is_signed<IT>::value, std::size_t>::type
Encode(IT n, BVec* v) {
  return EncodeSigned(n, v);
}

template<typename IT, class BVec=std::vector<std::uint8_t>>
inline typename std::enable_if<!std::is_signed<IT>::value, std::size_t>::type
Encode(IT n, BVec* v) {
  return EncodeUnsigned(n, v);
}

// class allowing to use any large enough buffer (usually a uint8_t* or char*) as argument to Encode*
// Example:
//   std::uint8_t buf[vle::LargeEnoughBuf::MaxLen<std::int16_t>()];
//   vle::EncodeSigned((std::int16_t) 42, LargeEnoughBuf(buf));
class LargeEnoughBuf {
 public:
  template<typename IT> static constexpr std::size_t MaxLen() { return (sizeof(IT) * 8 + 6) / 7; }

  explicit LargeEnoughBuf(void* buf): p((std::uint8_t*) buf) { }

  std::uint8_t* end() const { return p; }

  void push_back(uint8_t b) { *p++ = b; }

 private:
  std::uint8_t* p;
};

// by-value overloads for LargeEnoughBuf (thin pointer wrapper, safe to copy)
template<typename IT> inline std::size_t EncodeSigned(IT n, LargeEnoughBuf v) {
  return EncodeSigned(n, &v);
}
template<typename IT> inline std::size_t EncodeUnsigned(IT n, LargeEnoughBuf v) {
  return EncodeUnsigned(n, &v);
}
inline std::size_t EncodeDummySigned(LargeEnoughBuf v) {
  return EncodeDummySigned(&v);
}
inline std::size_t EncodeDummyUnsigned(LargeEnoughBuf v) {
  return EncodeDummyUnsigned(&v);
}
template<typename IT> inline std::size_t Encode(IT n, LargeEnoughBuf v) {
  return Encode(n, &v);
}

}  // namespace vle

#endif
