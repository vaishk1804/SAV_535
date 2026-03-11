#include "Cache.h"
#include <sstream>

Cache::Cache(Memory &memory) : memory_(memory), lines_(CACHE_LINES) {}

void Cache::reset()
{
  for (auto &line : lines_)
  {
    line.valid = false;
    line.dirty = false;
    line.tag = 0;
    line.data.fill(0);
  }
  hits_ = misses_ = reads_ = writes_ = 0;
}

uint32_t Cache::blockNumber(Address addr) const
{
  return (addr % RAM_WORDS) / WORDS_PER_LINE;
}

uint32_t Cache::lineIndex(Address addr) const
{
  return blockNumber(addr) % CACHE_LINES;
}

uint32_t Cache::tag(Address addr) const
{
  return blockNumber(addr) / CACHE_LINES;
}

uint32_t Cache::offset(Address addr) const
{
  return (addr % RAM_WORDS) % WORDS_PER_LINE;
}

Address Cache::blockBaseAddress(Address addr) const
{
  Address norm = addr % RAM_WORDS;
  return norm - (norm % WORDS_PER_LINE);
}

MemoryResponse Cache::read(Address address, Stage stage)
{
  reads_++;

  uint32_t idx = lineIndex(address);
  uint32_t tg = tag(address);
  CacheLine &line = lines_[idx];

  if (line.valid && line.tag == tg)
  {
    hits_++;
    LineData result;
    result.words = line.data;
    return {AccessStatus::DONE, result, "done: cache hit"};
  }

  misses_++;
  auto memRes = memory_.readLine(blockBaseAddress(address), stage);

  if (memRes.status == AccessStatus::WAIT)
  {
    return {AccessStatus::WAIT, {}, "wait: cache miss, waiting for memory line"};
  }
  if (memRes.status == AccessStatus::ERROR)
  {
    return {AccessStatus::ERROR, {}, memRes.message};
  }

  line.valid = true;
  line.dirty = false;
  line.tag = tg;
  line.data = memRes.line.words;

  LineData result;
  result.words = line.data;

  std::ostringstream oss;
  oss << "done: cache miss filled line";
  return {AccessStatus::DONE, result, oss.str()};
}

WriteResponse Cache::write(Address address, Word value, Stage stage)
{
  writes_++;

  uint32_t idx = lineIndex(address);
  uint32_t tg = tag(address);
  uint32_t off = offset(address);
  CacheLine &line = lines_[idx];

  if (line.valid && line.tag == tg)
  {
    hits_++;

    line.data[off] = value;
    line.dirty = false;

    auto memRes = memory_.writeWord(address, value, stage);
    if (memRes.status == AccessStatus::WAIT)
    {
      return {AccessStatus::WAIT, "wait: write hit, waiting for write-through"};
    }
    if (memRes.status == AccessStatus::ERROR)
    {
      return memRes;
    }
    return {AccessStatus::DONE, "done: write hit, cache updated and memory updated"};
  }

  misses_++;
  auto memRes = memory_.writeWord(address, value, stage);
  if (memRes.status == AccessStatus::WAIT)
  {
    return {AccessStatus::WAIT, "wait: write miss, no-write-allocate, memory write in progress"};
  }
  if (memRes.status == AccessStatus::ERROR)
  {
    return memRes;
  }
  return {AccessStatus::DONE, "done: write miss bypassed cache, memory updated"};
}

const CacheLine &Cache::viewLine(uint32_t lineIndexIn) const
{
  return lines_[lineIndexIn % CACHE_LINES];
}

uint64_t Cache::hits() const { return hits_; }
uint64_t Cache::misses() const { return misses_; }
uint64_t Cache::reads() const { return reads_; }
uint64_t Cache::writes() const { return writes_; }