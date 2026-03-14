#include <cstdint>
#include <limits>
#include <vector>

// criterion/alloc.h has an operator== with unused parameters.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <criterion/criterion.h>
#pragma GCC diagnostic pop

#include "vle.h"

using Bytes = std::vector<std::uint8_t>;

// --- EncodeUnsigned tests ---

Test(EncodeUnsigned, zero) {
  Bytes v;
  std::size_t sz = vle::EncodeUnsigned(0u, &v);
  cr_assert_eq(sz, 1);
  cr_assert_eq(v.size(), 1);
  cr_assert_eq(v[0], 0x00);
}

Test(EncodeUnsigned, one) {
  Bytes v;
  std::size_t sz = vle::EncodeUnsigned(1u, &v);
  cr_assert_eq(sz, 1);
  cr_assert_eq(v.size(), 1);
  cr_assert_eq(v[0], 0x01);
}

Test(EncodeUnsigned, max_single_byte) {
  // 127 = 0x7f, fits in one byte (7 payload bits, no continuation)
  Bytes v;
  std::size_t sz = vle::EncodeUnsigned(127u, &v);
  cr_assert_eq(sz, 1);
  cr_assert_eq(v[0], 0x7f);
}

Test(EncodeUnsigned, min_two_bytes) {
  // 128 needs 2 bytes: [0x81, 0x00]
  // first byte: cont=1, payload=0x01; second: cont=0, payload=0x00
  // value = (1 << 7) | 0 = 128
  Bytes v;
  std::size_t sz = vle::EncodeUnsigned(128u, &v);
  cr_assert_eq(sz, 2);
  cr_assert_eq(v[0], 0x81);
  cr_assert_eq(v[1], 0x00);
}

Test(EncodeUnsigned, value_300) {
  // 300 = 0x12C → [0x82, 0x2C]
  // first: cont=1, payload=0x02; second: cont=0, payload=0x2C
  // value = (2 << 7) | 0x2C = 256 + 44 = 300
  Bytes v;
  std::size_t sz = vle::EncodeUnsigned(300u, &v);
  cr_assert_eq(sz, 2);
  cr_assert_eq(v[0], 0x82);
  cr_assert_eq(v[1], 0x2c);
}

Test(EncodeUnsigned, max_two_bytes) {
  // 16383 = 0x3FFF → [0xFF, 0x7F]
  Bytes v;
  std::size_t sz = vle::EncodeUnsigned(16383u, &v);
  cr_assert_eq(sz, 2);
  cr_assert_eq(v[0], 0xff);
  cr_assert_eq(v[1], 0x7f);
}

Test(EncodeUnsigned, min_three_bytes) {
  // 16384 = 0x4000 → [0x81, 0x80, 0x00]
  Bytes v;
  std::size_t sz = vle::EncodeUnsigned(16384u, &v);
  cr_assert_eq(sz, 3);
  cr_assert_eq(v[0], 0x81);
  cr_assert_eq(v[1], 0x80);
  cr_assert_eq(v[2], 0x00);
}

Test(EncodeUnsigned, returns_size) {
  Bytes v;
  v.push_back(0xFF); // pre-existing byte
  std::size_t sz = vle::EncodeUnsigned(300u, &v);
  cr_assert_eq(sz, 2);
  cr_assert_eq(v.size(), 3); // 1 pre-existing + 2 encoded
}

Test(EncodeUnsigned, appends_to_existing) {
  Bytes v;
  std::size_t sz1 = vle::EncodeUnsigned(1u, &v);
  std::size_t sz2 = vle::EncodeUnsigned(128u, &v);
  cr_assert_eq(sz1, 1);
  cr_assert_eq(sz2, 2);
  cr_assert_eq(v.size(), 3);
  cr_assert_eq(v[0], 0x01);
  cr_assert_eq(v[1], 0x81);
  cr_assert_eq(v[2], 0x00);
}

Test(EncodeUnsigned, uint64_large) {
  // 2^14 = 16384 as uint64_t
  Bytes v;
  std::size_t sz = vle::EncodeUnsigned((std::uint64_t)16384, &v);
  cr_assert_eq(sz, 3);
  cr_assert_eq(v[0], 0x81);
  cr_assert_eq(v[1], 0x80);
  cr_assert_eq(v[2], 0x00);
}

// --- EncodeSigned tests ---

Test(EncodeSigned, zero) {
  Bytes v;
  std::size_t sz = vle::EncodeSigned(0, &v);
  cr_assert_eq(sz, 1);
  cr_assert_eq(v[0], 0x00);
}

Test(EncodeSigned, positive_one) {
  Bytes v;
  std::size_t sz = vle::EncodeSigned(1, &v);
  cr_assert_eq(sz, 1);
  cr_assert_eq(v[0], 0x01);
}

Test(EncodeSigned, max_single_byte_positive) {
  // 63 fits in 6 payload bits of first byte (sign=0)
  Bytes v;
  std::size_t sz = vle::EncodeSigned(63, &v);
  cr_assert_eq(sz, 1);
  cr_assert_eq(v[0], 0x3f);
}

Test(EncodeSigned, min_two_byte_positive) {
  // 64 requires 2 bytes: [0x80, 0x40]
  // first: cont=1, sign=0, payload=0x00; second: cont=0, payload=0x40
  // value = (0 << 7) | 64 = 64
  Bytes v;
  std::size_t sz = vle::EncodeSigned(64, &v);
  cr_assert_eq(sz, 2);
  cr_assert_eq(v[0], 0x80);
  cr_assert_eq(v[1], 0x40);
}

Test(EncodeSigned, negative_one) {
  // -1 → encode abs(-1)-1 = 0, signBit=0x40
  // push_back(0 | 0x40) = 0x40
  Bytes v;
  std::size_t sz = vle::EncodeSigned(-1, &v);
  cr_assert_eq(sz, 1);
  cr_assert_eq(v[0], 0x40);
}

Test(EncodeSigned, negative_two) {
  // -2 → encode 1, signBit=0x40 → 0x41
  Bytes v;
  std::size_t sz = vle::EncodeSigned(-2, &v);
  cr_assert_eq(sz, 1);
  cr_assert_eq(v[0], 0x41);
}

Test(EncodeSigned, negative_64) {
  // -64 → encode 63, signBit=0x40 → 63 | 0x40 = 0x7f
  Bytes v;
  std::size_t sz = vle::EncodeSigned(-64, &v);
  cr_assert_eq(sz, 1);
  cr_assert_eq(v[0], 0x7f);
}

Test(EncodeSigned, negative_65) {
  // -65 → encode 64 with signBit=0x40
  // bytes [0x40, 0xC0] → reverse → [0xC0, 0x40]
  Bytes v;
  std::size_t sz = vle::EncodeSigned(-65, &v);
  cr_assert_eq(sz, 2);
  cr_assert_eq(v[0], 0xc0);
  cr_assert_eq(v[1], 0x40);
}

Test(EncodeSigned, positive_appends) {
  Bytes v;
  std::size_t sz1 = vle::EncodeSigned(0, &v);
  std::size_t sz2 = vle::EncodeSigned(-1, &v);
  cr_assert_eq(sz1, 1);
  cr_assert_eq(sz2, 1);
  cr_assert_eq(v.size(), 2);
  cr_assert_eq(v[0], 0x00);
  cr_assert_eq(v[1], 0x40);
}

// --- Size consistency ---

Test(EncodeSize, unsigned_size_matches_vector) {
  for (unsigned val : {0u, 1u, 127u, 128u, 255u, 16383u, 16384u}) {
    Bytes v;
    std::size_t sz = vle::EncodeUnsigned(val, &v);
    cr_assert_eq(sz, v.size(), "size mismatch for value %u", val);
  }
}

Test(EncodeSize, signed_size_matches_vector) {
  for (int val : {0, 1, 63, 64, -1, -64, -65}) {
    Bytes v;
    std::size_t sz = vle::EncodeSigned(val, &v);
    cr_assert_eq(sz, v.size(), "size mismatch for value %d", val);
  }
}

// --- Continuation bit invariants ---

Test(Invariants, last_byte_has_no_continuation) {
  for (unsigned val : {0u, 1u, 127u, 128u, 300u, 16384u}) {
    Bytes v;
    vle::EncodeUnsigned(val, &v);
    cr_assert_eq(v.back() & 0x80, 0,
      "last byte should have continuation=0 for value %u", val);
  }
}

Test(Invariants, non_last_bytes_have_continuation) {
  Bytes v;
  vle::EncodeUnsigned(16384u, &v); // 3 bytes
  cr_assert(v[0] & 0x80, "first byte should have continuation=1");
  cr_assert(v[1] & 0x80, "middle byte should have continuation=1");
}

// --- EncodeDummy tests ---

Test(EncodeDummy, unsigned_bytes) {
  Bytes v;
  std::size_t sz = vle::EncodeDummyUnsigned(&v);
  cr_assert_eq(sz, 1);
  cr_assert_eq(v.size(), 1);
  cr_assert_eq(v[0], 0x80);
}

Test(EncodeDummy, signed_bytes) {
  Bytes v;
  std::size_t sz = vle::EncodeDummySigned(&v);
  cr_assert_eq(sz, 2);
  cr_assert_eq(v.size(), 2);
  cr_assert_eq(v[0], 0x80);
  cr_assert_eq(v[1], 0x00);
}

// --- DecodeUnsigned tests ---

Test(DecodeUnsigned, zero) {
  Bytes v;
  vle::EncodeUnsigned((unsigned)0, &v);
  auto dr = vle::DecodeUnsigned<unsigned>(v.data(), v.data() + v.size());
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.len, (ssize_t)v.size());
  cr_assert_eq(dr.n, 0u);
}

Test(DecodeUnsigned, single_byte) {
  Bytes v;
  vle::EncodeUnsigned((unsigned)127, &v);
  auto dr = vle::DecodeUnsigned<unsigned>(v.data(), v.data() + v.size());
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.len, (ssize_t)v.size());
  cr_assert_eq(dr.n, 127u);
}

Test(DecodeUnsigned, multi_byte) {
  Bytes v;
  vle::EncodeUnsigned((unsigned)300, &v);
  auto dr = vle::DecodeUnsigned<unsigned>(v.data(), v.data() + v.size());
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.len, (ssize_t)v.size());
  cr_assert_eq(dr.n, 300u);
}

Test(DecodeUnsigned, incomplete_empty) {
  auto dr = vle::DecodeUnsigned<unsigned>((std::uint8_t*)nullptr, (std::uint8_t*)nullptr);
  cr_assert_eq(dr.len, vle::INCOMPLETE);
}


Test(DecodeUnsigned, dummy) {
  Bytes v;
  vle::EncodeDummyUnsigned(&v);
  auto dr = vle::DecodeUnsigned<unsigned>(v.data(), v.data() + v.size());
  cr_assert_eq(dr.len, vle::DUMMY);
}

// --- DecodeSigned tests ---

Test(DecodeSigned, zero) {
  Bytes v;
  vle::EncodeSigned(0, &v);
  auto dr = vle::DecodeSigned<int>(v.data(), v.data() + v.size());
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.len, (ssize_t)v.size());
  cr_assert_eq(dr.n, 0);
}

Test(DecodeSigned, positive) {
  Bytes v;
  vle::EncodeSigned(64, &v);
  auto dr = vle::DecodeSigned<int>(v.data(), v.data() + v.size());
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.len, (ssize_t)v.size());
  cr_assert_eq(dr.n, 64);
}

Test(DecodeSigned, negative) {
  Bytes v;
  vle::EncodeSigned(-65, &v);
  auto dr = vle::DecodeSigned<int>(v.data(), v.data() + v.size());
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.len, (ssize_t)v.size());
  cr_assert_eq(dr.n, -65);
}

Test(DecodeSigned, incomplete_empty) {
  auto dr = vle::DecodeSigned<int>((std::uint8_t*)nullptr, (std::uint8_t*)nullptr);
  cr_assert_eq(dr.len, vle::INCOMPLETE);
}


Test(DecodeSigned, dummy) {
  Bytes v;
  vle::EncodeDummySigned(&v);
  auto dr = vle::DecodeSigned<int>(v.data(), v.data() + v.size());
  cr_assert_eq(dr.len, vle::DUMMY);
}

// --- Roundtrip: all uint8_t values ---

Test(Roundtrip, all_uint8) {
  for (int i = 0; i <= std::numeric_limits<std::uint8_t>::max(); ++i) {
    std::uint8_t val = (std::uint8_t)i;
    Bytes v;
    vle::EncodeUnsigned(val, &v);
    auto dr = vle::DecodeUnsigned<std::uint8_t>(v.data(), v.data() + v.size());
    cr_assert_gt(dr.len, 0,
      "decode len <= 0 for uint8 value %d", i);
    cr_assert_eq(dr.len, (ssize_t)v.size(),
      "decode len mismatch for uint8 value %d", i);
    cr_assert_eq(dr.n, val,
      "roundtrip mismatch for uint8 value %d", i);
    for (std::size_t len = v.size() - 1; len > 0; --len) {
      auto tr = vle::DecodeUnsigned<std::uint8_t>(v.data(), v.data() + len);
      cr_assert_eq(tr.len, vle::INCOMPLETE,
        "expected INCOMPLETE for uint8 value %d truncated to %zu bytes", i, len);
    }
  }
}

// --- Roundtrip: all uint16_t values ---

Test(Roundtrip, all_uint16) {
  for (int i = 0; i <= std::numeric_limits<std::uint16_t>::max(); ++i) {
    std::uint16_t val = (std::uint16_t)i;
    Bytes v;
    vle::EncodeUnsigned(val, &v);
    auto dr = vle::DecodeUnsigned<std::uint16_t>(v.data(), v.data() + v.size());
    cr_assert_gt(dr.len, 0,
      "decode len <= 0 for uint16 value %d", i);
    cr_assert_eq(dr.len, (ssize_t)v.size(),
      "decode len mismatch for uint16 value %d", i);
    cr_assert_eq(dr.n, val,
      "roundtrip mismatch for uint16 value %d", i);
    for (std::size_t len = v.size() - 1; len > 0; --len) {
      auto tr = vle::DecodeUnsigned<std::uint16_t>(v.data(), v.data() + len);
      cr_assert_eq(tr.len, vle::INCOMPLETE,
        "expected INCOMPLETE for uint16 value %d truncated to %zu bytes", i, len);
    }
  }
}

// --- Roundtrip: all int8_t values ---

Test(Roundtrip, all_int8) {
  for (int i = std::numeric_limits<std::int8_t>::min();
       i <= std::numeric_limits<std::int8_t>::max(); ++i) {
    std::int8_t val = (std::int8_t)i;
    Bytes v;
    vle::EncodeSigned(val, &v);
    auto dr = vle::DecodeSigned<std::int8_t>(v.data(), v.data() + v.size());
    cr_assert_gt(dr.len, 0,
      "decode len <= 0 for int8 value %d", i);
    cr_assert_eq(dr.len, (ssize_t)v.size(),
      "decode len mismatch for int8 value %d", i);
    cr_assert_eq(dr.n, val,
      "roundtrip mismatch for int8 value %d", i);
    for (std::size_t len = v.size() - 1; len > 0; --len) {
      auto tr = vle::DecodeSigned<std::int8_t>(v.data(), v.data() + len);
      cr_assert_eq(tr.len, vle::INCOMPLETE,
        "expected INCOMPLETE for int8 value %d truncated to %zu bytes", i, len);
    }
  }
}

// --- Roundtrip: all int16_t values ---

Test(Roundtrip, all_int16) {
  for (int i = std::numeric_limits<std::int16_t>::min();
       i <= std::numeric_limits<std::int16_t>::max(); ++i) {
    std::int16_t val = (std::int16_t)i;
    Bytes v;
    vle::EncodeSigned(val, &v);
    auto dr = vle::DecodeSigned<std::int16_t>(v.data(), v.data() + v.size());
    cr_assert_gt(dr.len, 0,
      "decode len <= 0 for int16 value %d", i);
    cr_assert_eq(dr.len, (ssize_t)v.size(),
      "decode len mismatch for int16 value %d", i);
    cr_assert_eq(dr.n, val,
      "roundtrip mismatch for int16 value %d", i);
    for (std::size_t len = v.size() - 1; len > 0; --len) {
      auto tr = vle::DecodeSigned<std::int16_t>(v.data(), v.data() + len);
      cr_assert_eq(tr.len, vle::INCOMPLETE,
        "expected INCOMPLETE for int16 value %d truncated to %zu bytes", i, len);
    }
  }
}

// --- Roundtrip: largest unsigned type max ---

Test(Roundtrip, uintmax_max) {
  std::uintmax_t val = std::numeric_limits<std::uintmax_t>::max();
  Bytes v;
  vle::EncodeUnsigned(val, &v);
  auto dr = vle::DecodeUnsigned<std::uintmax_t>(v.data(), v.data() + v.size());
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.len, (ssize_t)v.size());
  cr_assert_eq(dr.n, val);
  cr_assert_leq(v.size(), vle::LargeEnoughBuf::MaxLen<std::uintmax_t>());
}

// --- Roundtrip: largest signed type max and min ---

Test(Roundtrip, intmax_max) {
  std::intmax_t val = std::numeric_limits<std::intmax_t>::max();
  Bytes v;
  vle::EncodeSigned(val, &v);
  auto dr = vle::DecodeSigned<std::intmax_t>(v.data(), v.data() + v.size());
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.len, (ssize_t)v.size());
  cr_assert_eq(dr.n, val);
  cr_assert_leq(v.size(), vle::LargeEnoughBuf::MaxLen<std::intmax_t>());
}

Test(Roundtrip, intmax_min) {
  std::intmax_t val = std::numeric_limits<std::intmax_t>::min();
  Bytes v;
  vle::EncodeSigned(val, &v);
  auto dr = vle::DecodeSigned<std::intmax_t>(v.data(), v.data() + v.size());
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.len, (ssize_t)v.size());
  cr_assert_eq(dr.n, val);
  cr_assert_leq(v.size(), vle::LargeEnoughBuf::MaxLen<std::intmax_t>());
}

// --- Dummy roundtrip with specific types ---

Test(DummyRoundtrip, unsigned_dummy_decode) {
  Bytes v;
  vle::EncodeDummyUnsigned(&v);
  auto dr = vle::DecodeUnsigned<std::uint64_t>(v.data(), v.data() + v.size());
  cr_assert_eq(dr.len, vle::DUMMY);
}

Test(DummyRoundtrip, signed_dummy_decode) {
  Bytes v;
  vle::EncodeDummySigned(&v);
  auto dr = vle::DecodeSigned<std::int64_t>(v.data(), v.data() + v.size());
  cr_assert_eq(dr.len, vle::DUMMY);
}

Test(DummyRoundtrip, signed_dummy_decode_int8) {
  Bytes v;
  vle::EncodeDummySigned(&v);
  auto dr = vle::DecodeSigned<std::int8_t>(v.data(), v.data() + v.size());
  cr_assert_eq(dr.len, vle::DUMMY);
}

Test(DummyRoundtrip, signed_dummy_decode_intmax) {
  Bytes v;
  vle::EncodeDummySigned(&v);
  auto dr = vle::DecodeSigned<std::intmax_t>(v.data(), v.data() + v.size());
  cr_assert_eq(dr.len, vle::DUMMY);
}

// --- Encode (auto-dispatch) tests ---

Test(Encode, unsigned_value) {
  Bytes v1, v2;
  std::size_t sz1 = vle::Encode(300u, &v1);
  std::size_t sz2 = vle::EncodeUnsigned(300u, &v2);
  cr_assert_eq(sz1, sz2);
  cr_assert_eq(v1, v2);
}

Test(Encode, signed_positive) {
  Bytes v1, v2;
  std::size_t sz1 = vle::Encode(64, &v1);
  std::size_t sz2 = vle::EncodeSigned(64, &v2);
  cr_assert_eq(sz1, sz2);
  cr_assert_eq(v1, v2);
}

Test(Encode, signed_negative) {
  Bytes v1, v2;
  std::size_t sz1 = vle::Encode(-65, &v1);
  std::size_t sz2 = vle::EncodeSigned(-65, &v2);
  cr_assert_eq(sz1, sz2);
  cr_assert_eq(v1, v2);
}

Test(Encode, unsigned_zero) {
  Bytes v1, v2;
  vle::Encode(0u, &v1);
  vle::EncodeUnsigned(0u, &v2);
  cr_assert_eq(v1, v2);
}

Test(Encode, signed_zero) {
  Bytes v1, v2;
  vle::Encode(0, &v1);
  vle::EncodeSigned(0, &v2);
  cr_assert_eq(v1, v2);
}

Test(Encode, uint64) {
  Bytes v1, v2;
  std::uint64_t val = 16384;
  vle::Encode(val, &v1);
  vle::EncodeUnsigned(val, &v2);
  cr_assert_eq(v1, v2);
}

Test(Encode, int64_negative) {
  Bytes v1, v2;
  std::int64_t val = -1000;
  vle::Encode(val, &v1);
  vle::EncodeSigned(val, &v2);
  cr_assert_eq(v1, v2);
}

Test(Encode, large_enough_buf_unsigned) {
  std::uint8_t buf1[vle::LargeEnoughBuf::MaxLen<unsigned>()];
  std::uint8_t buf2[vle::LargeEnoughBuf::MaxLen<unsigned>()];
  std::size_t sz1 = vle::Encode(300u, vle::LargeEnoughBuf(buf1));
  std::size_t sz2 = vle::EncodeUnsigned(300u, vle::LargeEnoughBuf(buf2));
  cr_assert_eq(sz1, sz2);
  for (std::size_t i = 0; i < sz1; ++i)
    cr_assert_eq(buf1[i], buf2[i]);
}

Test(Encode, large_enough_buf_signed) {
  std::uint8_t buf1[vle::LargeEnoughBuf::MaxLen<int>()];
  std::uint8_t buf2[vle::LargeEnoughBuf::MaxLen<int>()];
  std::size_t sz1 = vle::Encode(-65, vle::LargeEnoughBuf(buf1));
  std::size_t sz2 = vle::EncodeSigned(-65, vle::LargeEnoughBuf(buf2));
  cr_assert_eq(sz1, sz2);
  for (std::size_t i = 0; i < sz1; ++i)
    cr_assert_eq(buf1[i], buf2[i]);
}

// --- LargeEnoughBuf tests ---

Test(LargeEnoughBuf, encode_unsigned) {
  std::uint8_t buf[vle::LargeEnoughBuf::MaxLen<unsigned>()];
  vle::LargeEnoughBuf leb(buf);
  std::size_t sz = vle::EncodeUnsigned(300u, &leb);
  auto dr = vle::DecodeUnsigned<unsigned>(buf, buf + sz);
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.n, 300u);
}

Test(LargeEnoughBuf, encode_signed) {
  std::uint8_t buf[vle::LargeEnoughBuf::MaxLen<int>()];
  vle::LargeEnoughBuf leb(buf);
  std::size_t sz = vle::EncodeSigned(-65, &leb);
  auto dr = vle::DecodeSigned<int>(buf, buf + sz);
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.n, -65);
}

Test(LargeEnoughBuf, encode_unsigned_max) {
  std::uint8_t buf[vle::LargeEnoughBuf::MaxLen<std::uintmax_t>()];
  vle::LargeEnoughBuf leb(buf);
  std::uintmax_t val = std::numeric_limits<std::uintmax_t>::max();
  std::size_t sz = vle::EncodeUnsigned(val, &leb);
  cr_assert_leq(sz, sizeof(buf));
  auto dr = vle::DecodeUnsigned<std::uintmax_t>(buf, buf + sz);
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.n, val);
}

Test(LargeEnoughBuf, encode_signed_min) {
  std::uint8_t buf[vle::LargeEnoughBuf::MaxLen<std::intmax_t>()];
  vle::LargeEnoughBuf leb(buf);
  std::intmax_t val = std::numeric_limits<std::intmax_t>::min();
  std::size_t sz = vle::EncodeSigned(val, &leb);
  cr_assert_leq(sz, sizeof(buf));
  auto dr = vle::DecodeSigned<std::intmax_t>(buf, buf + sz);
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.n, val);
}

// --- LargeEnoughBuf by-value tests ---

Test(LargeEnoughBuf, by_value_unsigned) {
  std::uint8_t buf[vle::LargeEnoughBuf::MaxLen<unsigned>()];
  std::size_t sz = vle::EncodeUnsigned(300u, vle::LargeEnoughBuf(buf));
  auto dr = vle::DecodeUnsigned<unsigned>(buf, buf + sz);
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.n, 300u);
}

Test(LargeEnoughBuf, by_value_signed) {
  std::uint8_t buf[vle::LargeEnoughBuf::MaxLen<int>()];
  std::size_t sz = vle::EncodeSigned(-65, vle::LargeEnoughBuf(buf));
  auto dr = vle::DecodeSigned<int>(buf, buf + sz);
  cr_assert_gt(dr.len, 0);
  cr_assert_eq(dr.n, -65);
}

Test(LargeEnoughBuf, by_value_dummy_unsigned) {
  std::uint8_t buf[1];
  std::size_t sz = vle::EncodeDummyUnsigned(vle::LargeEnoughBuf(buf));
  cr_assert_eq(sz, 1);
  auto dr = vle::DecodeUnsigned<unsigned>(buf, buf + sz);
  cr_assert_eq(dr.len, vle::DUMMY);
}

Test(LargeEnoughBuf, by_value_dummy_signed) {
  std::uint8_t buf[2];
  std::size_t sz = vle::EncodeDummySigned(vle::LargeEnoughBuf(buf));
  cr_assert_eq(sz, 2);
  auto dr = vle::DecodeSigned<int>(buf, buf + sz);
  cr_assert_eq(dr.len, vle::DUMMY);
}
