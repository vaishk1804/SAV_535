#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

using Word = int32_t;
using Address = uint32_t;

constexpr int WORDS_PER_LINE = 4;
constexpr int RAM_WORDS = 32768;
constexpr int CACHE_LINES = 8;
constexpr int MEMORY_DELAY = 3;

enum class Stage
{
  NONE,
  IF_STAGE,
  MEM_STAGE
};

inline std::string stageToString(Stage s)
{
  switch (s)
  {
  case Stage::IF_STAGE:
    return "IF";
  case Stage::MEM_STAGE:
    return "MEM";
  default:
    return "NONE";
  }
}

inline Stage parseStage(const std::string &s)
{
  if (s == "IF")
    return Stage::IF_STAGE;
  if (s == "MEM")
    return Stage::MEM_STAGE;
  return Stage::NONE;
}

enum class AccessType
{
  READ_LINE,
  WRITE_WORD
};

enum class AccessStatus
{
  WAIT,
  DONE,
  ERROR
};

struct LineData
{
  std::array<Word, WORDS_PER_LINE> words{};
};

struct MemoryResponse
{
  AccessStatus status = AccessStatus::ERROR;
  LineData line{};
  std::string message;
};

struct WriteResponse
{
  AccessStatus status = AccessStatus::ERROR;
  std::string message;
};

struct CacheLine
{
  bool valid = false;
  bool dirty = false;
  uint32_t tag = 0;
  std::array<Word, WORDS_PER_LINE> data{};
};

struct PendingMemoryRequest
{
  bool active = false;
  Stage stage = Stage::NONE;
  AccessType type = AccessType::READ_LINE;
  Address address = 0;
  Word writeValue = 0;
  int remaining = 0;
};

inline std::string wordToHex(Word value)
{
  std::ostringstream oss;
  oss << "0x"
      << std::uppercase
      << std::hex
      << std::setw(8)
      << std::setfill('0')
      << static_cast<uint32_t>(value);
  return oss.str();
}

inline std::string lineToHexString(const LineData &line)
{
  std::ostringstream oss;
  oss << "["
      << wordToHex(line.words[0]) << ", "
      << wordToHex(line.words[1]) << ", "
      << wordToHex(line.words[2]) << ", "
      << wordToHex(line.words[3]) << "]";
  return oss.str();
}