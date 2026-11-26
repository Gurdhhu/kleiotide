#include "gui/zip_entry.hpp"
#include "third_party/miniz/miniz_tinfl.h"
#include <algorithm>
#include <stdexcept>

namespace kleiotide {
namespace {
[[noreturn]] void invalid_zip(const char* message) {
  throw std::runtime_error(message);
}
}
QByteArray zip_entry(const QByteArray &zip, const QByteArray &name, quint32 limit) {
  auto u16 = [&](qsizetype at) -> quint16 {
    if (at < 0 || at > zip.size() - 2)
      invalid_zip("Truncated ZIP archive.");
    return quint16(quint8(zip.at(at))) |
           quint16(quint16(quint8(zip.at(at + 1))) << 8);
  };
  auto u32 = [&](qsizetype at) -> quint32 {
    if (at < 0 || at > zip.size() - 4)
      invalid_zip("Truncated ZIP archive.");
    return quint32(quint8(zip.at(at))) |
           (quint32(quint8(zip.at(at + 1))) << 8) |
           (quint32(quint8(zip.at(at + 2))) << 16) |
           (quint32(quint8(zip.at(at + 3))) << 24);
  };
  qsizetype end = -1;
  for (qsizetype at = zip.size() - 22;
       at >= std::max(qsizetype(0), zip.size() - 65557); --at)
    if (u32(at) == 0x06054b50 && at + 22 + u16(at + 20) == zip.size()) {
      end = at;
      break;
    }
  if (end < 0 || u16(end + 4) || u16(end + 6) || u16(end + 8) != u16(end + 10))
    invalid_zip("Unsupported or invalid ZIP archive.");
  qsizetype at = u32(end + 16);
  for (unsigned n = 0; n < u16(end + 10); ++n) {
    if (u32(at) != 0x02014b50)
      invalid_zip("Invalid ZIP directory.");
    const auto method = u16(at + 10), flags = u16(at + 8);
    const quint32 size = u32(at + 24), packed = u32(at + 20),
                  crc = u32(at + 16);
    const auto length = u16(at + 28);
    const auto local = u32(at + 42);
    if (at + 46 + length > zip.size())
      invalid_zip("Truncated ZIP directory.");
    if (zip.mid(at + 46, length) == name) {
      if (quint64(local) + 30 > quint64(zip.size()) || flags & 1 ||
          size > limit || packed > limit ||
          u32(local) != 0x04034b50)
        invalid_zip("Unsupported or oversized ZIP entry.");
      const quint64 offset =
          quint64(local) + 30 + u16(local + 26) + u16(local + 28);
      if (offset + packed > quint64(zip.size()))
        invalid_zip("Truncated ZIP entry.");
      QByteArray out;
      if (method == 0) {
        out = zip.mid(qsizetype(offset), packed);
        if (out.size() != size)
          invalid_zip("Invalid ZIP entry length.");
      } else if (method == 8) {
        out.resize(size);
        const auto decoded = tinfl_decompress_mem_to_mem(
            out.data(), size, zip.constData() + offset, packed, 0);
        if (decoded != size)
          invalid_zip("Could not decompress ZIP entry.");
      } else
        invalid_zip("Unsupported ZIP compression.");
      quint32 sum = 0xffffffffU;
      for (unsigned char b : out) {
        sum ^= b;
        for (int bit = 0; bit < 8; ++bit)
          sum = (sum >> 1) ^ ((sum & 1U) ? 0xedb88320U : 0U);
      }
      if (~sum != crc)
        invalid_zip("Damaged ZIP entry (checksum mismatch).");
      return out;
    }
    at += 46 + length + u16(at + 30) + u16(at + 32);
  }
  return {};
}
}
