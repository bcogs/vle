# vle

A single-header C++11 library for variable-length encoding of integers.

## Format

- The most significant bit of each byte is a **continuation flag**:
  `1` means more bytes follow, `0` means this is the last byte.
- The remaining 7 bits of each byte carry payload in **big-endian** order:
  the first byte holds the most significant payload bits,
  the last byte holds the least significant.
- **Unsigned** numbers: all payload bits represent the value directly.
- **Signed** numbers: the second most significant bit of the first byte
  is a **sign flag** (`1` = negative), leaving 6 payload bits in the first byte.
  The encoded value is the integer itself when non-negative,
  or `abs(integer) - 1` when negative.
- A **dummy** value (representing "no value") is encoded as `0x80` for unsigned
  and `0x80 0x00` for signed.

## Usage

Copy `vle.h` into your project and `#include "vle.h"`.

### Encoding into a vector

```cpp
#include "vle.h"
#include <vector>
#include <cstdint>

std::vector<std::uint8_t> buf;

// Unsigned
vle::EncodeUnsigned<std::uint32_t>(300, &buf);   // appends 2 bytes

// Signed
vle::EncodeSigned<int>(-65, &buf);      // appends 2 bytes

// Auto-dispatch: calls EncodeSigned or EncodeUnsigned based on the type
vle::Encode<std::int32_t>(-65, &buf);     // signed → EncodeSigned
vle::Encode<std::uint32_t>(300, &buf);    // unsigned → EncodeUnsigned

// Dummy (sentinel for "no value")
vle::EncodeDummyUnsigned(&buf);    // appends 1 byte
vle::EncodeDummySigned(&buf);      // appends 2 bytes
```

### Encoding into a fixed-size buffer with `LargeEnoughBuf`

`LargeEnoughBuf` wraps a raw pointer so you can encode directly into a
stack-allocated buffer, avoiding heap allocation. Use `MaxLen<T>()` to
determine the maximum number of bytes any value of type `T` can produce.

```cpp
#include "vle.h"
#include <cstdint>

// Stack buffer guaranteed large enough for any int32_t
std::uint8_t buf[vle::LargeEnoughBuf::MaxLen<std::int32_t>()];

// Encode by passing LargeEnoughBuf by value (thin pointer wrapper)
std::size_t n = vle::EncodeSigned(-12345, vle::LargeEnoughBuf(buf));
// buf[0..n) now contains the encoded bytes
```

### Decoding

Decode functions take a pair of `start`/`end` pointers (or iterators).
They return a `vle::Result<T>` with two fields:

- `len`: bytes consumed (`> 0`), or `vle::INCOMPLETE` / `vle::DUMMY`
- `n`: the decoded value (valid only when `len > 0`)

```cpp
#include "vle.h"
#include <cstdint>
#include <vector>

std::vector<std::uint8_t> buf;
vle::EncodeSigned(300, &buf);

vle::Result<int> r = vle::DecodeSigned<int>(buf.data(), buf.data() + buf.size());
if (r.len > 0) {
    // r.n == 300
} else if (r.len == vle::INCOMPLETE) {
    // need more bytes
} else if (r.len == vle::DUMMY) {
    // dummy / sentinel value
}
```

### Decoding a sequence of values

```cpp
const std::uint8_t* p   = buf.data();
const std::uint8_t* end = p + buf.size();

while (p < end) {
    auto r = vle::DecodeUnsigned<std::uint64_t>(p, end);
    if (r.len <= 0) break;  // INCOMPLETE or DUMMY
    process(r.n);
    p += r.len;
}
```
