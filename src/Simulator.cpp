#include "Simulator.h"
#include <sstream>

Simulator::Simulator() : memory_(), cache_(memory_)
{
  reset();
}

void Simulator::reset()
{
  memory_.reset();
  cache_.reset();
  cycles_ = 0;
}

std::string Simulator::handleRead(Address address, Stage stage)
{
  if (stage == Stage::NONE)
  {
    return "ERROR: stage must be IF or MEM";
  }

  cycles_++;
  auto res = cache_.read(address, stage);

  std::ostringstream oss;
  oss << "[cycle " << cycles_ << "] "
      << "R " << address << " " << stageToString(stage) << " -> ";

  if (res.status == AccessStatus::WAIT)
  {
    oss << "wait";
  }
  else if (res.status == AccessStatus::DONE)
  {
    uint32_t off = (address % RAM_WORDS) % WORDS_PER_LINE;
    oss << "done, line=" << lineToHexString(res.line)
        << ", requested_word=" << wordToHex(res.line.words[off]);
  }
  else
  {
    oss << "error";
  }

  oss << " | " << res.message;
  return oss.str();
}

std::string Simulator::handleWrite(Word value, Address address, Stage stage)
{
  if (stage == Stage::NONE)
  {
    return "ERROR: stage must be IF or MEM";
  }

  cycles_++;
  auto res = cache_.write(address, value, stage);

  std::ostringstream oss;
  oss << "[cycle " << cycles_ << "] "
      << "W " << wordToHex(value) << " " << address << " " << stageToString(stage) << " -> ";

  if (res.status == AccessStatus::WAIT)
  {
    oss << "wait";
  }
  else if (res.status == AccessStatus::DONE)
  {
    oss << "done";
  }
  else
  {
    oss << "error";
  }

  oss << " | " << res.message;
  return oss.str();
}

std::string Simulator::handleView(int level, uint32_t line) const
{
  std::ostringstream oss;

  if (level == 0)
  {
    auto ramLine = memory_.viewLine(line);
    oss << "V 0 " << line << " -> RAM line " << line
        << " = " << lineToHexString(ramLine);
    return oss.str();
  }

  if (level == 1)
  {
    const auto &c = cache_.viewLine(line);

    LineData cacheLineData;
    cacheLineData.words = c.data;

    oss << "V 1 " << line << " -> CACHE line " << (line % CACHE_LINES)
        << " | tag=" << wordToHex(static_cast<Word>(c.tag))
        << " valid=" << (c.valid ? 1 : 0)
        << " dirty=" << (c.dirty ? 1 : 0)
        << " data=" << lineToHexString(cacheLineData);
    return oss.str();
  }

  return "ERROR: level must be 0 (RAM) or 1 (cache)";
}

std::string Simulator::handleHelp() const
{
  return "Commands:\n"
         "  R <address> <stage>         e.g. R 12 IF\n"
         "  W <value> <address> <stage> e.g. W 99 12 MEM\n"
         "  V <level> <line>            level 0=RAM, 1=cache\n"
         "  S                           show status\n"
         "  H                           help\n"
         "  Q                           quit";
}

std::string Simulator::handleStatus() const
{
  std::ostringstream oss;
  oss << "Status:\n"
      << "  command_cycles=" << cycles_ << "\n"
      << "  memory_busy=" << (memory_.busy() ? "YES" : "NO") << "\n"
      << "  memory_active_stage=" << stageToString(memory_.activeStage()) << "\n"
      << "  memory_remaining=" << memory_.remainingCycles() << "\n"
      << "  cache_hits=" << cache_.hits() << "\n"
      << "  cache_misses=" << cache_.misses() << "\n"
      << "  cache_reads=" << cache_.reads() << "\n"
      << "  cache_writes=" << cache_.writes();
  return oss.str();
}