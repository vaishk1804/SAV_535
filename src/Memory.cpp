#include "Memory.h"
#include <sstream>

Memory::Memory() : ram_(RAM_WORDS, 0) {}

void Memory::reset()
{
  std::fill(ram_.begin(), ram_.end(), 0);
  pending_ = PendingMemoryRequest{};
}

Address Memory::normalize(Address addr) const
{
  return static_cast<Address>(addr % RAM_WORDS);
}

uint32_t Memory::lineNumberFromAddress(Address addr) const
{
  return normalize(addr) / WORDS_PER_LINE;
}

uint32_t Memory::numLines() const
{
  return RAM_WORDS / WORDS_PER_LINE;
}

MemoryResponse Memory::readLine(Address address, Stage stage)
{
  if (stage == Stage::NONE)
  {
    return {AccessStatus::ERROR, {}, "invalid stage"};
  }

  Address norm = normalize(address);

  if (!pending_.active)
  {
    pending_.active = true;
    pending_.stage = stage;
    pending_.type = AccessType::READ_LINE;
    pending_.address = norm;
    pending_.remaining = MEMORY_DELAY;
    return {AccessStatus::WAIT, {}, "wait: started memory read request"};
  }

  if (pending_.stage != stage ||
      pending_.type != AccessType::READ_LINE ||
      pending_.address != norm)
  {
    return {AccessStatus::WAIT, {}, "wait: memory busy, request not advanced"};
  }

  pending_.remaining--;

  if (pending_.remaining > 0)
  {
    std::ostringstream oss;
    oss << "wait: memory read in progress, remaining=" << pending_.remaining;
    return {AccessStatus::WAIT, {}, oss.str()};
  }

  uint32_t lineNum = lineNumberFromAddress(norm);
  uint32_t base = lineNum * WORDS_PER_LINE;

  LineData out;
  for (int i = 0; i < WORDS_PER_LINE; ++i)
  {
    out.words[i] = ram_[base + i];
  }

  pending_ = PendingMemoryRequest{};
  return {AccessStatus::DONE, out, "done: memory returned full line"};
}

WriteResponse Memory::writeWord(Address address, Word value, Stage stage)
{
  if (stage == Stage::NONE)
  {
    return {AccessStatus::ERROR, "invalid stage"};
  }

  Address norm = normalize(address);

  if (!pending_.active)
  {
    pending_.active = true;
    pending_.stage = stage;
    pending_.type = AccessType::WRITE_WORD;
    pending_.address = norm;
    pending_.writeValue = value;
    pending_.remaining = MEMORY_DELAY;
    return {AccessStatus::WAIT, "wait: started memory write request"};
  }

  if (pending_.stage != stage ||
      pending_.type != AccessType::WRITE_WORD ||
      pending_.address != norm ||
      pending_.writeValue != value)
  {
    return {AccessStatus::WAIT, "wait: memory busy, request not advanced"};
  }

  pending_.remaining--;

  if (pending_.remaining > 0)
  {
    std::ostringstream oss;
    oss << "wait: memory write in progress, remaining=" << pending_.remaining;
    return {AccessStatus::WAIT, oss.str()};
  }

  ram_[norm] = value;
  pending_ = PendingMemoryRequest{};
  return {AccessStatus::DONE, "done: memory write completed"};
}

LineData Memory::viewLine(uint32_t lineNumber) const
{
  LineData out;
  uint32_t wrappedLine = lineNumber % numLines();
  uint32_t base = wrappedLine * WORDS_PER_LINE;
  for (int i = 0; i < WORDS_PER_LINE; ++i)
  {
    out.words[i] = ram_[base + i];
  }
  return out;
}

Word Memory::peekWord(Address address) const
{
  return ram_[normalize(address)];
}

bool Memory::busy() const
{
  return pending_.active;
}

Stage Memory::activeStage() const
{
  return pending_.stage;
}

int Memory::remainingCycles() const
{
  return pending_.remaining;
}