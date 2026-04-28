#pragma once
#include "Assembler.h"
#include "Cache.h"
#include "Instruction.h"
#include "L2Cache.h"
#include "Memory.h"
#include "Types.h"
#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

enum class ExecMode
{
    NO_PIPE_NO_CACHE = 0,
    NO_PIPE_CACHE = 1,
    PIPE_NO_CACHE = 2,
    PIPE_CACHE = 3
};

struct StatusRegister
{
    bool Z = false;
};

struct PipeReg
{
    bool valid = false;
    uint32_t pc = 0;
    uint32_t raw = 0;
    Instruction inst{};

    int32_t opA = 0;
    int32_t opB = 0;
    int32_t exResult = 0;
    int32_t memAddr = 0;
    int32_t memData = 0;
    int32_t wbValue = 0;

    bool loadStarted = false;
    bool storeStarted = false;

    std::string note;
};

struct SimulatorSnapshot
{
    uint64_t cycles = 0;
    uint32_t pc = 0;
    bool halted = false;
    bool haltRequested = false;
    bool faulted = false;
    bool zFlag = false;
    std::string mode;
    std::array<int32_t, 16> regs{};

    std::string ifStage;
    std::string idStage;
    std::string exStage;
    std::string memStage;
    std::string wbStage;

    std::string summary;
    std::string flags;
    std::string hierarchyState;
    std::string seqState;

    bool fetchInFlight = false;
    uint32_t fetchPC = 0;
    uint32_t fetchRemaining = 0;

    std::vector<SnapshotCacheRow> l1Rows;
    std::vector<SnapshotCacheRow> l2Rows;
    std::vector<MemoryRow> memoryRows;

    uint64_t l1Hits = 0;
    uint64_t l1Misses = 0;
    uint64_t l2Hits = 0;
    uint64_t l2Misses = 0;
};

class Simulator
{
public:
    Simulator();

    void reset();
    std::string loadProgramAsm(const std::string &filename);
    std::string loadProgramWords(const std::vector<uint32_t> &words);

    void setMode(ExecMode mode);
    ExecMode mode() const { return mode_; }

    std::string step();
    std::string run(uint64_t maxCycles = 500000);
    std::string runUntilBreakpoint(uint64_t maxCycles = 500000);

    void addBreakpoint(uint32_t addr);
    void clearBreakpoint(uint32_t addr);
    void clearBreakpoints();
    std::vector<uint32_t> listBreakpoints() const;

    SimulatorSnapshot getSnapshot(uint32_t memStart = 0, uint32_t memLines = 16) const;

    std::string dumpRegs() const;
    std::string dumpPipeline() const;
    std::string dumpMemoryRange(Address start, Address count) const;
    std::string dumpCacheL1() const;
    std::string dumpCacheL2() const;
    std::string handleStatus() const;

private:
    Memory memory_;
    L2Cache l2_;
    Cache l1_;

    uint64_t cycles_ = 0;
    uint32_t pc_ = 0;
    bool halted_ = false;
    bool haltRequested_ = false;
    bool faulted_ = false;
    bool programEndReached_ = false;
    StatusRegister status_{};
    std::array<int32_t, 16> regs_{};

    uint32_t programBase_ = 0;
    uint32_t programSize_ = 0;
    ExecMode mode_ = ExecMode::PIPE_CACHE;

    std::vector<uint32_t> loadedProgramWords_;

    PipeReg IF_, ID_, EX_, MEM_, WB_;

    struct HierarchyPort
    {
        bool busy = false;
        Stage owner = Stage::IF_STAGE;
        uint32_t remaining = 0;
        HierarchyResult result{};
        enum class Kind
        {
            NONE,
            FETCH,
            LOAD,
            STORE,
            SEQ_FETCH,
            SEQ_LOAD,
            SEQ_STORE
        } kind = Kind::NONE;
        Address address = 0;
        Word writeValue = 0;
        uint32_t producerPc = 0;
        int destReg = 0;
        uint64_t epoch = 0;
    } hierarchyPort_;

    uint64_t fetchEpoch_ = 1;

    struct SeqState
    {
        enum class Phase
        {
            IDLE,
            FETCH_WAIT,
            EXECUTE,
            MEM_WAIT,
            WRITEBACK,
            HALTED
        } phase = Phase::IDLE;
        uint32_t pc = 0;
        uint32_t raw = 0;
        Instruction inst{};
        int32_t aluResult = 0;
        int32_t memAddr = 0;
        int32_t memData = 0;
        int32_t wbValue = 0;
        bool loadStarted = false;
        bool storeStarted = false;
        std::string note;
    } seq_;

    std::set<uint32_t> breakpoints_;

    bool stallThisCycle_ = false;
    bool squashThisCycle_ = false;
    std::string lastSummary_;
    std::string lastFlags_;
    std::string faultMessage_;

    uint32_t fetchInstructionWord(uint32_t address, bool cachesEnabled, bool &stall, std::string &msg);
    Instruction fetchAndDecode(uint32_t address, bool cachesEnabled, bool &stall, std::string &msg);

    void stepSequentialCycle(bool cachesEnabled);
    void stepPipelineCycle(bool cachesEnabled);

    HierarchyResult accessReadWord(Address address, bool cachesEnabled);
    HierarchyResult accessWriteWord(Address address, Word value, bool cachesEnabled);

    bool startHierarchyRequest(Stage owner,
                               HierarchyPort::Kind kind,
                               Address address,
                               bool cachesEnabled,
                               Word writeValue = 0,
                               int destReg = 0,
                               uint32_t producerPc = 0,
                               uint64_t epoch = 0);

    void advanceHierarchy();

    void doWB();
    void doMEM(bool cachesEnabled, bool &stall);
    void doEX(bool &squash, uint32_t &newPC);
    void doID(bool &stall);
    void doIF(bool cachesEnabled, bool stall);

    int32_t readReg(int reg) const;
    void writeReg(int reg, int32_t value);
    void updateZeroFlagIfNeeded(const Instruction &inst, int32_t value);

    bool pendingDestInPipe(int reg) const;
    bool hasRawHazard(const Instruction &inst) const;
    std::string modeString() const;

    int32_t evalALU(const Instruction &inst, int32_t a, int32_t b) const;
    bool evalBranch(const Instruction &inst, int32_t a, int32_t b) const;

    std::string pipeToString(const PipeReg &p) const;
    std::string seqPhaseString() const;
    std::string hierarchyString() const;

    bool pcInLoadedProgram(uint32_t pc) const;
    uint32_t programEnd() const;
    void fault(const std::string &message);

    bool cachesEnabledForCurrentMode() const;
};