#include "Simulator.h"
#include "UI.h"
#include <climits>
#include <sstream>
#include <stdexcept>
constexpr uint64_t SEQUENTIAL_EXTRA_STAGE_COST = 2;

namespace
{
    int32_t add32(int32_t a, int32_t b)
    {
        return static_cast<int32_t>(static_cast<uint32_t>(a) + static_cast<uint32_t>(b));
    }

    int32_t sub32(int32_t a, int32_t b)
    {
        return static_cast<int32_t>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b));
    }

    int32_t mul32(int32_t a, int32_t b)
    {
        int64_t wide = static_cast<int64_t>(a) * static_cast<int64_t>(b);
        return static_cast<int32_t>(static_cast<uint32_t>(wide));
    }

    int32_t div32(int32_t a, int32_t b)
    {
        if (b == 0)
            return 0;
        if (a == INT32_MIN && b == -1)
            return INT32_MIN;
        return a / b;
    }

    int32_t mod32(int32_t a, int32_t b)
    {
        if (b == 0)
            return 0;
        if (a == INT32_MIN && b == -1)
            return 0;
        return a % b;
    }

    int32_t sll32(int32_t a, int32_t sh)
    {
        return static_cast<int32_t>(static_cast<uint32_t>(a) << (sh & 0x1F));
    }

    int32_t srl32(int32_t a, int32_t sh)
    {
        return static_cast<int32_t>(static_cast<uint32_t>(a) >> (sh & 0x1F));
    }

    int32_t sra32(int32_t a, int32_t sh)
    {
        return static_cast<int32_t>(a >> (sh & 0x1F));
    }
}

Simulator::Simulator() : l2_(memory_), l1_(l2_)
{
    reset();
}

void Simulator::reset()
{
    memory_.reset();
    l2_.reset();
    l1_.reset();

    regs_.fill(0);
    status_ = {};
    cycles_ = 0;
    pc_ = 0;
    halted_ = false;
    haltRequested_ = false;
    faulted_ = false;
    programEndReached_ = false;

    programBase_ = 0;
    programSize_ = 0;

    IF_ = {};
    ID_ = {};
    EX_ = {};
    MEM_ = {};
    WB_ = {};

    hierarchyPort_ = {};
    fetchEpoch_ = 1;

    seq_ = {};
    breakpoints_.clear();

    stallThisCycle_ = false;
    squashThisCycle_ = false;
    lastSummary_.clear();
    lastFlags_.clear();
    faultMessage_.clear();
}

bool Simulator::pcInLoadedProgram(uint32_t pc) const
{
    return pc >= programBase_ && pc < (programBase_ + programSize_);
}

uint32_t Simulator::programEnd() const
{
    return programBase_ + programSize_;
}

void Simulator::fault(const std::string &message)
{
    faulted_ = true;
    halted_ = true;
    haltRequested_ = false;
    programEndReached_ = false;

    hierarchyPort_ = {};
    IF_ = {};
    ID_ = {};
    EX_ = {};
    MEM_ = {};
    WB_ = {};

    lastSummary_ = message;
    lastFlags_ = "FAULT";
    faultMessage_ = message;
}

bool Simulator::cachesEnabledForCurrentMode() const
{
    return mode_ == ExecMode::NO_PIPE_CACHE || mode_ == ExecMode::PIPE_CACHE;
}

std::string Simulator::loadProgramWords(const std::vector<uint32_t> &words)
{
    loadedProgramWords_ = words;

    reset();
    programBase_ = 0;
    programSize_ = static_cast<uint32_t>(words.size());

    for (uint32_t i = 0; i < words.size(); ++i)
    {
        memory_.pokeWord(programBase_ + i, words[i]);
    }

    pc_ = programBase_;
    seq_.pc = programBase_;
    lastSummary_ = "Program loaded";
    return "Program loaded";
}

std::string Simulator::loadProgramAsm(const std::string &filename)
{
    auto assembled = Assembler::assembleFile(filename);
    return loadProgramWords(assembled.words);
}

void Simulator::setMode(ExecMode mode)
{
    mode_ = mode;

    if (!loadedProgramWords_.empty())
    {
        auto saved = loadedProgramWords_;
        loadProgramWords(saved);
        lastSummary_ = "Mode set and program reloaded";
    }
    else
    {
        lastSummary_ = "Mode set";
    }
}

void Simulator::addBreakpoint(uint32_t addr)
{
    breakpoints_.insert(addr);
}

void Simulator::clearBreakpoint(uint32_t addr)
{
    breakpoints_.erase(addr);
}

void Simulator::clearBreakpoints()
{
    breakpoints_.clear();
}

std::vector<uint32_t> Simulator::listBreakpoints() const
{
    return std::vector<uint32_t>(breakpoints_.begin(), breakpoints_.end());
}

std::string Simulator::modeString() const
{
    switch (mode_)
    {
    case ExecMode::NO_PIPE_NO_CACHE:
        return "NO_PIPE_NO_CACHE";
    case ExecMode::NO_PIPE_CACHE:
        return "NO_PIPE_CACHE";
    case ExecMode::PIPE_NO_CACHE:
        return "PIPE_NO_CACHE";
    case ExecMode::PIPE_CACHE:
        return "PIPE_CACHE";
    }
    return "UNKNOWN";
}

int32_t Simulator::readReg(int reg) const
{
    if (reg <= 0 || reg > 15)
        return 0;
    return regs_[reg];
}

void Simulator::writeReg(int reg, int32_t value)
{
    if (reg == 0)
        return;
    if (reg >= 0 && reg < 16)
        regs_[reg] = value;
}

void Simulator::updateZeroFlagIfNeeded(const Instruction &inst, int32_t value)
{
    if (inst.updatesZeroFlag())
    {
        status_.Z = (value == 0);
    }
}

int32_t Simulator::evalALU(const Instruction &inst, int32_t a, int32_t b) const
{
    switch (inst.op)
    {
    case Opcode::ADD:
        return add32(a, b);
    case Opcode::SUB:
        return sub32(a, b);
    case Opcode::MUL:
        return mul32(a, b);
    case Opcode::DIV:
        return div32(a, b);
    case Opcode::MOD:
        return mod32(a, b);
    case Opcode::AND:
        return static_cast<int32_t>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    case Opcode::OR:
        return static_cast<int32_t>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    case Opcode::XOR:
        return static_cast<int32_t>(static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
    case Opcode::NOT:
        return static_cast<int32_t>(~static_cast<uint32_t>(a));
    case Opcode::SLL:
        return sll32(a, b);
    case Opcode::SRL:
        return srl32(a, b);
    case Opcode::SRA:
        return sra32(a, b);
    case Opcode::SLT:
        return (a < b) ? 1 : 0;
    case Opcode::ADDI:
        return add32(a, inst.imm);
    default:
        return 0;
    }
}

bool Simulator::evalBranch(const Instruction &inst, int32_t a, int32_t b) const
{
    switch (inst.op)
    {
    case Opcode::BEQ:
        return a == b;
    case Opcode::BNE:
        return a != b;
    case Opcode::BLT:
        return a < b;
    case Opcode::BGE:
        return a >= b;
    default:
        return false;
    }
}

HierarchyResult Simulator::accessReadWord(Address address, bool cachesEnabled)
{
    if (cachesEnabled)
    {
        return l1_.readWord(address);
    }

    HierarchyResult out;
    out.hit = false;
    out.l2Hit = false;
    out.word = memory_.peekWord(address);
    out.latency = DRAM_LATENCY;
    out.message = "Memory read";
    return out;
}

HierarchyResult Simulator::accessWriteWord(Address address, Word value, bool cachesEnabled)
{
    if (cachesEnabled)
    {
        return l1_.writeWord(address, value);
    }

    memory_.pokeWord(address, value);
    HierarchyResult out;
    out.word = value;
    out.latency = DRAM_LATENCY;
    out.message = "Memory write";
    return out;
}

bool Simulator::startHierarchyRequest(Stage owner,
                                      HierarchyPort::Kind kind,
                                      Address address,
                                      bool cachesEnabled,
                                      Word writeValue,
                                      int destReg,
                                      uint32_t producerPc,
                                      uint64_t epoch)
{
    if (hierarchyPort_.busy)
        return false;

    hierarchyPort_.busy = true;
    hierarchyPort_.owner = owner;
    hierarchyPort_.kind = kind;
    hierarchyPort_.address = address;
    hierarchyPort_.writeValue = writeValue;
    hierarchyPort_.destReg = destReg;
    hierarchyPort_.producerPc = producerPc;
    hierarchyPort_.epoch = epoch;

    hierarchyPort_.result =
        (kind == HierarchyPort::Kind::STORE || kind == HierarchyPort::Kind::SEQ_STORE)
            ? accessWriteWord(address, writeValue, cachesEnabled)
            : accessReadWord(address, cachesEnabled);

    hierarchyPort_.remaining = hierarchyPort_.result.latency;
    return true;
}

void Simulator::advanceHierarchy()
{
    if (!hierarchyPort_.busy)
        return;

    if (hierarchyPort_.remaining > 0)
    {
        --hierarchyPort_.remaining;
    }
    if (hierarchyPort_.remaining != 0)
        return;

    switch (hierarchyPort_.kind)
    {
    case HierarchyPort::Kind::FETCH:
        if (hierarchyPort_.epoch == fetchEpoch_)
        {
            IF_.valid = true;
            IF_.pc = hierarchyPort_.producerPc;
            IF_.raw = hierarchyPort_.result.word;
            IF_.inst = decodeInstruction(hierarchyPort_.result.word);
            IF_.note = hierarchyPort_.result.hit ? "[Fetched hit]" : "[Fetched]";
        }
        break;

    case HierarchyPort::Kind::LOAD:
        MEM_.wbValue = static_cast<int32_t>(hierarchyPort_.result.word);
        MEM_.note = "[Load complete]";
        break;

    case HierarchyPort::Kind::STORE:
        MEM_.note = "[Store complete]";
        break;

    case HierarchyPort::Kind::SEQ_FETCH:
        seq_.raw = hierarchyPort_.result.word;
        seq_.inst = decodeInstruction(seq_.raw);
        seq_.phase = SeqState::Phase::EXECUTE;
        seq_.note = "Sequential fetch complete";
        break;

    case HierarchyPort::Kind::SEQ_LOAD:
        seq_.wbValue = static_cast<int32_t>(hierarchyPort_.result.word);
        seq_.phase = SeqState::Phase::WRITEBACK;
        seq_.note = "Sequential load complete";
        break;

    case HierarchyPort::Kind::SEQ_STORE:
        seq_.phase = SeqState::Phase::WRITEBACK;
        seq_.note = "Sequential store complete";
        break;

    case HierarchyPort::Kind::NONE:
        break;
    }

    hierarchyPort_ = {};
}

bool Simulator::pendingDestInPipe(int reg) const
{
    if (reg == 0)
        return false;

    auto matches = [&](const PipeReg &p)
    {
        return p.valid && p.inst.writesRegister() &&
               ((p.inst.op == Opcode::JAL ? 15 : p.inst.rd) == reg);
    };

    return matches(EX_) || matches(MEM_);
}

bool Simulator::hasRawHazard(const Instruction &inst) const
{
    if (inst.readsRs1() && pendingDestInPipe(inst.rs1))
        return true;
    if (inst.readsRs2() && pendingDestInPipe(inst.rs2))
        return true;
    if (inst.isStore() && pendingDestInPipe(inst.rd))
        return true;
    if (inst.isJump() && pendingDestInPipe(inst.rs1))
        return true;
    return false;
}

uint32_t Simulator::fetchInstructionWord(uint32_t address, bool cachesEnabled, bool &stall, std::string &msg)
{
    auto res = accessReadWord(address, cachesEnabled);
    stall = false;
    msg = res.message;
    return res.word;
}

Instruction Simulator::fetchAndDecode(uint32_t address, bool cachesEnabled, bool &stall, std::string &msg)
{
    return decodeInstruction(fetchInstructionWord(address, cachesEnabled, stall, msg));
}

void Simulator::doWB()
{
    if (!WB_.valid || faulted_)
        return;

    if (WB_.inst.writesRegister())
    {
        const int dest = (WB_.inst.op == Opcode::JAL ? 15 : WB_.inst.rd);
        writeReg(dest, WB_.wbValue);
        updateZeroFlagIfNeeded(WB_.inst, WB_.wbValue);
    }

    if (WB_.inst.op == Opcode::HALT)
    {
        haltRequested_ = true;
        lastSummary_ = "HALT retired";
    }

    WB_ = {};
}

void Simulator::doMEM(bool cachesEnabled, bool &stall)
{
    stall = false;
    if (!MEM_.valid || faulted_)
        return;

    if (MEM_.inst.isLoad())
    {
        if (!MEM_.loadStarted)
        {
            if (!hierarchyPort_.busy)
            {
                if (!startHierarchyRequest(Stage::MEM_STAGE,
                                           HierarchyPort::Kind::LOAD,
                                           static_cast<Address>(MEM_.memAddr),
                                           cachesEnabled,
                                           0,
                                           MEM_.inst.rd,
                                           MEM_.pc,
                                           fetchEpoch_))
                {
                    stall = true;
                    stallThisCycle_ = true;
                    lastSummary_ = "MEM stalled starting load";
                    lastFlags_ = "STALL";
                    return;
                }
                MEM_.loadStarted = true;
                MEM_.note = "[Load wait]";
            }
            stall = true;
            stallThisCycle_ = true;
            lastSummary_ = "MEM waiting on load";
            lastFlags_ = "STALL";
            return;
        }

        if (hierarchyPort_.busy && hierarchyPort_.owner == Stage::MEM_STAGE)
        {
            stall = true;
            stallThisCycle_ = true;
            lastSummary_ = "MEM waiting on load";
            lastFlags_ = "STALL";
            return;
        }

        WB_ = MEM_;
        MEM_ = {};
        return;
    }

    if (MEM_.inst.isStore())
    {
        if (!MEM_.storeStarted)
        {
            if (!hierarchyPort_.busy)
            {
                if (!startHierarchyRequest(Stage::MEM_STAGE,
                                           HierarchyPort::Kind::STORE,
                                           static_cast<Address>(MEM_.memAddr),
                                           cachesEnabled,
                                           static_cast<Word>(MEM_.memData),
                                           0,
                                           MEM_.pc,
                                           fetchEpoch_))
                {
                    stall = true;
                    stallThisCycle_ = true;
                    lastSummary_ = "MEM stalled starting store";
                    lastFlags_ = "STALL";
                    return;
                }
                MEM_.storeStarted = true;
                MEM_.note = "[Store wait]";
            }
            stall = true;
            stallThisCycle_ = true;
            lastSummary_ = "MEM waiting on store";
            lastFlags_ = "STALL";
            return;
        }

        if (hierarchyPort_.busy && hierarchyPort_.owner == Stage::MEM_STAGE)
        {
            stall = true;
            stallThisCycle_ = true;
            lastSummary_ = "MEM waiting on store";
            lastFlags_ = "STALL";
            return;
        }

        MEM_ = {};
        return;
    }

    WB_ = MEM_;
    MEM_ = {};
}

void Simulator::doEX(bool &squash, uint32_t &newPC)
{
    squash = false;
    newPC = pc_;

    if (!EX_.valid || faulted_)
        return;

    auto &inst = EX_.inst;
    const int32_t a = EX_.opA;
    const int32_t b = EX_.opB;

    if (inst.op == Opcode::HALT)
    {
        EX_.note = "[HALT in EX]";
        MEM_ = EX_;
        EX_ = {};
        return;
    }

    if (inst.isBranch())
    {
        const bool taken = evalBranch(inst, a, b);
        if (taken)
        {
            const uint32_t target = EX_.pc + 1 + static_cast<uint32_t>(inst.imm);
            if (!pcInLoadedProgram(target))
            {
                fault("Branch target outside program: PC=" + std::to_string(target));
                return;
            }
            squash = true;
            squashThisCycle_ = true;
            newPC = target;
            EX_.note = "[Branch taken]";
            lastSummary_ = "Branch taken -> squash";
            lastFlags_ = "SQUASH";
        }
        else
        {
            EX_.note = "[Branch not taken]";
        }
        EX_ = {};
        return;
    }

    if (inst.op == Opcode::J)
    {
        const uint32_t target = static_cast<uint32_t>(a);
        if (!pcInLoadedProgram(target))
        {
            fault("Jump target outside program: PC=" + std::to_string(target));
            return;
        }
        squash = true;
        squashThisCycle_ = true;
        newPC = target;
        EX_.note = "[Jump taken]";
        lastSummary_ = "Jump taken -> squash";
        lastFlags_ = "SQUASH";
        EX_ = {};
        return;
    }

    if (inst.op == Opcode::JAL)
    {
        const uint32_t target = static_cast<uint32_t>(a);
        if (!pcInLoadedProgram(target))
        {
            fault("JAL target outside program: PC=" + std::to_string(target));
            return;
        }
        EX_.wbValue = static_cast<int32_t>(EX_.pc + 1);
        squash = true;
        squashThisCycle_ = true;
        newPC = target;
        EX_.note = "[JAL taken]";
        lastSummary_ = "JAL taken -> squash";
        lastFlags_ = "SQUASH";
        MEM_ = EX_;
        EX_ = {};
        return;
    }

    if (inst.isLoad() || inst.isStore())
    {
        EX_.memAddr = add32(a, inst.imm);
        if (inst.isStore())
        {
            EX_.memData = readReg(inst.rd);
        }
        EX_.note = "[Address computed]";
        MEM_ = EX_;
        EX_ = {};
        return;
    }

    EX_.wbValue = evalALU(inst, a, b);
    EX_.note = "[ALU done]";
    MEM_ = EX_;
    EX_ = {};
}

void Simulator::doID(bool &stall)
{
    stall = false;
    if (!ID_.valid || faulted_)
        return;

    if (hasRawHazard(ID_.inst))
    {
        stall = true;
        stallThisCycle_ = true;
        ID_.note = "[STALL: RAW hazard]";
        lastSummary_ = "ID stalled on RAW hazard";
        lastFlags_ = "STALL";
        return;
    }

    ID_.opA = readReg(ID_.inst.rs1);
    ID_.opB = readReg(ID_.inst.rs2);
    ID_.note = "[Decoded]";
    EX_ = ID_;
    ID_ = {};
}

void Simulator::doIF(bool cachesEnabled, bool stall)
{
    if (stall || halted_ || haltRequested_ || faulted_ || IF_.valid || hierarchyPort_.busy)
        return;

    const uint32_t end = programEnd();

    if (pc_ == end)
    {
        programEndReached_ = true;
        lastSummary_ = "Program end reached, no more fetches";
        return;
    }

    if (!pcInLoadedProgram(pc_))
    {
        fault("PC escaped loaded program: PC=" + std::to_string(pc_));
        return;
    }

    startHierarchyRequest(Stage::IF_STAGE,
                          HierarchyPort::Kind::FETCH,
                          pc_,
                          cachesEnabled,
                          0,
                          0,
                          pc_,
                          fetchEpoch_);
}

void Simulator::stepPipelineCycle(bool cachesEnabled)
{
    if (faulted_)
        return;

    lastSummary_ = "Cycle OK";
    lastFlags_.clear();
    stallThisCycle_ = false;
    squashThisCycle_ = false;

    advanceHierarchy();
    if (faulted_)
        return;

    PipeReg nextIF = IF_;
    PipeReg nextID = ID_;
    PipeReg nextEX = EX_;
    PipeReg nextMEM = MEM_;
    PipeReg nextWB = WB_;

    if (WB_.valid)
    {
        if (WB_.inst.writesRegister())
        {
            const int dest = (WB_.inst.op == Opcode::JAL ? 15 : WB_.inst.rd);
            writeReg(dest, WB_.wbValue);
            updateZeroFlagIfNeeded(WB_.inst, WB_.wbValue);
        }

        if (WB_.inst.op == Opcode::HALT)
        {
            haltRequested_ = true;
            lastSummary_ = "HALT retired";
        }

        nextWB = {};
    }

    bool memStall = false;
    if (MEM_.valid)
    {
        if (MEM_.inst.isLoad())
        {
            if (!MEM_.loadStarted)
            {
                if (!hierarchyPort_.busy)
                {
                    if (!startHierarchyRequest(Stage::MEM_STAGE,
                                               HierarchyPort::Kind::LOAD,
                                               static_cast<Address>(MEM_.memAddr),
                                               cachesEnabled,
                                               0,
                                               MEM_.inst.rd,
                                               MEM_.pc,
                                               fetchEpoch_))
                    {
                        memStall = true;
                        stallThisCycle_ = true;
                        lastSummary_ = "MEM stalled starting load";
                        lastFlags_ = "STALL";
                    }
                    else
                    {
                        nextMEM.loadStarted = true;
                        nextMEM.note = "[Load wait]";
                        memStall = true;
                        stallThisCycle_ = true;
                        lastSummary_ = "MEM waiting on load";
                        lastFlags_ = "STALL";
                    }
                }
                else
                {
                    memStall = true;
                    stallThisCycle_ = true;
                    lastSummary_ = "MEM waiting on load";
                    lastFlags_ = "STALL";
                }
            }
            else if (hierarchyPort_.busy && hierarchyPort_.owner == Stage::MEM_STAGE)
            {
                memStall = true;
                stallThisCycle_ = true;
                lastSummary_ = "MEM waiting on load";
                lastFlags_ = "STALL";
            }
            else
            {
                nextWB = MEM_;
                nextMEM = {};
            }
        }
        else if (MEM_.inst.isStore())
        {
            if (!MEM_.storeStarted)
            {
                if (!hierarchyPort_.busy)
                {
                    if (!startHierarchyRequest(Stage::MEM_STAGE,
                                               HierarchyPort::Kind::STORE,
                                               static_cast<Address>(MEM_.memAddr),
                                               cachesEnabled,
                                               static_cast<Word>(MEM_.memData),
                                               0,
                                               MEM_.pc,
                                               fetchEpoch_))
                    {
                        memStall = true;
                        stallThisCycle_ = true;
                        lastSummary_ = "MEM stalled starting store";
                        lastFlags_ = "STALL";
                    }
                    else
                    {
                        nextMEM.storeStarted = true;
                        nextMEM.note = "[Store wait]";
                        memStall = true;
                        stallThisCycle_ = true;
                        lastSummary_ = "MEM waiting on store";
                        lastFlags_ = "STALL";
                    }
                }
                else
                {
                    memStall = true;
                    stallThisCycle_ = true;
                    lastSummary_ = "MEM waiting on store";
                    lastFlags_ = "STALL";
                }
            }
            else if (hierarchyPort_.busy && hierarchyPort_.owner == Stage::MEM_STAGE)
            {
                memStall = true;
                stallThisCycle_ = true;
                lastSummary_ = "MEM waiting on store";
                lastFlags_ = "STALL";
            }
            else
            {
                nextMEM = {};
            }
        }
        else
        {
            nextWB = MEM_;
            nextMEM = {};
        }
    }

    if (!memStall)
    {
        bool squash = false;
        uint32_t newPC = pc_;

        if (EX_.valid)
        {
            auto inst = EX_.inst;
            const int32_t a = EX_.opA;
            const int32_t b = EX_.opB;

            if (inst.op == Opcode::HALT)
            {
                PipeReg tmp = EX_;
                tmp.note = "[HALT in EX]";
                nextMEM = tmp;
                nextEX = {};
            }
            else if (inst.isBranch())
            {
                const bool taken = evalBranch(inst, a, b);
                if (taken)
                {
                    const uint32_t target = EX_.pc + 1 + static_cast<uint32_t>(inst.imm);
                    if (!pcInLoadedProgram(target))
                    {
                        fault("Branch target outside program: PC=" + std::to_string(target));
                        return;
                    }
                    squash = true;
                    squashThisCycle_ = true;
                    newPC = target;
                    lastSummary_ = "Branch taken -> squash";
                    lastFlags_ = "SQUASH";
                }
                nextEX = {};
            }
            else if (inst.op == Opcode::J)
            {
                const uint32_t target = static_cast<uint32_t>(a);
                if (!pcInLoadedProgram(target))
                {
                    fault("Jump target outside program: PC=" + std::to_string(target));
                    return;
                }
                squash = true;
                squashThisCycle_ = true;
                newPC = target;
                lastSummary_ = "Jump taken -> squash";
                lastFlags_ = "SQUASH";
                nextEX = {};
            }
            else if (inst.op == Opcode::JAL)
            {
                const uint32_t target = static_cast<uint32_t>(a);
                if (!pcInLoadedProgram(target))
                {
                    fault("JAL target outside program: PC=" + std::to_string(target));
                    return;
                }
                PipeReg tmp = EX_;
                tmp.wbValue = static_cast<int32_t>(EX_.pc + 1);
                tmp.note = "[JAL taken]";
                squash = true;
                squashThisCycle_ = true;
                newPC = target;
                lastSummary_ = "JAL taken -> squash";
                lastFlags_ = "SQUASH";
                nextMEM = tmp;
                nextEX = {};
            }
            else if (inst.isLoad() || inst.isStore())
            {
                PipeReg tmp = EX_;
                tmp.memAddr = add32(a, inst.imm);
                if (inst.isStore())
                {
                    tmp.memData = readReg(inst.rd);
                }
                tmp.note = "[Address computed]";
                nextMEM = tmp;
                nextEX = {};
            }
            else
            {
                PipeReg tmp = EX_;
                tmp.wbValue = evalALU(inst, a, b);
                tmp.note = "[ALU done]";
                nextMEM = tmp;
                nextEX = {};
            }
        }

        if (squash)
        {
            nextIF = {};
            nextID = {};
            nextEX = {};
            pc_ = newPC;
            programEndReached_ = false;

            if (hierarchyPort_.busy && hierarchyPort_.owner == Stage::IF_STAGE)
            {
                hierarchyPort_ = {};
            }
            ++fetchEpoch_;
        }
        else
        {
            bool idStall = false;

            if (ID_.valid)
            {
                if (hasRawHazard(ID_.inst))
                {
                    idStall = true;
                    stallThisCycle_ = true;
                    nextID.note = "[STALL: RAW hazard]";
                    lastSummary_ = "ID stalled on RAW hazard";
                    lastFlags_ = "STALL";
                }
                else
                {
                    PipeReg tmp = ID_;
                    tmp.opA = readReg(ID_.inst.rs1);
                    tmp.opB = readReg(ID_.inst.rs2);
                    tmp.note = "[Decoded]";
                    nextEX = tmp;
                    nextID = {};
                }
            }

            if (!idStall && !IF_.valid && !hierarchyPort_.busy && !haltRequested_ && !faulted_)
            {
                const uint32_t end = programEnd();
                if (pc_ == end)
                {
                    programEndReached_ = true;
                    lastSummary_ = "Program end reached, no more fetches";
                }
                else if (!pcInLoadedProgram(pc_))
                {
                    fault("PC escaped loaded program: PC=" + std::to_string(pc_));
                    return;
                }
                else
                {
                    startHierarchyRequest(Stage::IF_STAGE,
                                          HierarchyPort::Kind::FETCH,
                                          pc_,
                                          cachesEnabled,
                                          0,
                                          0,
                                          pc_,
                                          fetchEpoch_);
                }
            }

            if (!idStall && !ID_.valid && IF_.valid)
            {
                PipeReg tmp = IF_;
                tmp.note = "[Fetched->ID]";
                nextID = tmp;
                nextIF = {};
                ++pc_;
            }
        }
    }

    IF_ = nextIF;
    ID_ = nextID;
    EX_ = nextEX;
    MEM_ = nextMEM;
    WB_ = nextWB;

    if (haltRequested_ &&
        !IF_.valid && !ID_.valid && !EX_.valid && !MEM_.valid && !WB_.valid &&
        !hierarchyPort_.busy)
    {
        halted_ = true;
        lastSummary_ = "HALT retired";
        lastFlags_.clear();
    }

    if (programEndReached_ &&
        !IF_.valid && !ID_.valid && !EX_.valid && !MEM_.valid && !WB_.valid &&
        !hierarchyPort_.busy)
    {
        halted_ = true;
        lastSummary_ = "Program completed";
        lastFlags_.clear();
    }

    ++cycles_;
    regs_[0] = 0;
}

void Simulator::stepSequentialCycle(bool cachesEnabled)
{
    if (faulted_)
        return;

    lastFlags_.clear();
    stallThisCycle_ = false;
    squashThisCycle_ = false;

    advanceHierarchy();
    if (faulted_)
        return;

    switch (seq_.phase)
    {
    case SeqState::Phase::IDLE:
        if (halted_)
        {
            seq_.phase = SeqState::Phase::HALTED;
            break;
        }

        seq_.pc = pc_;
        if (seq_.pc == programEnd())
        {
            programEndReached_ = true;
            halted_ = true;
            seq_.phase = SeqState::Phase::HALTED;
            lastSummary_ = "Program completed";
            lastFlags_.clear();
            break;
        }

        if (!pcInLoadedProgram(seq_.pc))
        {
            fault("Sequential PC escaped program: PC=" + std::to_string(seq_.pc));
            return;
        }

        if (!hierarchyPort_.busy)
        {
            startHierarchyRequest(Stage::SEQ_STAGE,
                                  HierarchyPort::Kind::SEQ_FETCH,
                                  seq_.pc,
                                  cachesEnabled);
            seq_.phase = SeqState::Phase::FETCH_WAIT;
            seq_.note = "Sequential fetch";
            lastSummary_ = "Sequential fetch";
        }
        break;

    case SeqState::Phase::FETCH_WAIT:
        lastSummary_ = "Sequential fetch wait";
        break;

    case SeqState::Phase::EXECUTE:
    {
        // Non-pipelined mode should still represent the instruction
        // passing through the equivalent five stages one at a time.
        //
        // The existing sequential model already charges:
        // - IF through the fetch request/wait
        // - EX through this EXECUTE phase
        // - MEM through memory wait for LW/SW
        // - WB through WRITEBACK
        //
        // It was too optimistic for cache-only mode because it skipped
        // the explicit decode/empty-stage overhead. This extra cost keeps
        // non-pipeline timing fair against pipeline timing.

        cycles_ += SEQUENTIAL_EXTRA_STAGE_COST;

        auto &inst = seq_.inst;
        if (inst.op == Opcode::HALT)
        {
            halted_ = true;
            seq_.phase = SeqState::Phase::HALTED;
            seq_.note = "Sequential HALT";
            lastSummary_ = "Sequential HALT";
            lastFlags_.clear();
            break;
        }

        const int32_t a = readReg(inst.rs1);
        const int32_t b = readReg(inst.rs2);

        if (inst.isBranch())
        {
            uint32_t target = seq_.pc + 1;
            if (evalBranch(inst, a, b))
            {
                target = seq_.pc + 1 + static_cast<uint32_t>(inst.imm);
            }
            if (!pcInLoadedProgram(target))
            {
                fault("Sequential branch target outside program: PC=" + std::to_string(target));
                return;
            }
            pc_ = target;
            seq_.phase = SeqState::Phase::IDLE;
            seq_.note = "Sequential branch";
            lastSummary_ = seq_.note;
            break;
        }

        if (inst.op == Opcode::J)
        {
            const uint32_t target = static_cast<uint32_t>(a);
            if (!pcInLoadedProgram(target))
            {
                fault("Sequential jump target outside program: PC=" + std::to_string(target));
                return;
            }
            pc_ = target;
            seq_.phase = SeqState::Phase::IDLE;
            seq_.note = "Sequential jump";
            lastSummary_ = seq_.note;
            break;
        }

        if (inst.op == Opcode::JAL)
        {
            const uint32_t target = static_cast<uint32_t>(a);
            if (!pcInLoadedProgram(target))
            {
                fault("Sequential JAL target outside program: PC=" + std::to_string(target));
                return;
            }
            writeReg(15, static_cast<int32_t>(seq_.pc + 1));
            pc_ = target;
            seq_.phase = SeqState::Phase::IDLE;
            seq_.note = "Sequential JAL";
            lastSummary_ = seq_.note;
            break;
        }

        if (inst.isLoad())
        {
            seq_.memAddr = add32(a, inst.imm);
            if (!seq_.loadStarted && !hierarchyPort_.busy)
            {
                startHierarchyRequest(Stage::SEQ_STAGE,
                                      HierarchyPort::Kind::SEQ_LOAD,
                                      static_cast<Address>(seq_.memAddr),
                                      cachesEnabled,
                                      0,
                                      inst.rd,
                                      seq_.pc);
                seq_.loadStarted = true;
                seq_.phase = SeqState::Phase::MEM_WAIT;
                seq_.note = "Sequential load wait";
                lastSummary_ = seq_.note;
            }
            break;
        }

        if (inst.isStore())
        {
            seq_.memAddr = add32(a, inst.imm);
            seq_.memData = readReg(inst.rd);
            if (!seq_.storeStarted && !hierarchyPort_.busy)
            {
                startHierarchyRequest(Stage::SEQ_STAGE,
                                      HierarchyPort::Kind::SEQ_STORE,
                                      static_cast<Address>(seq_.memAddr),
                                      cachesEnabled,
                                      static_cast<Word>(seq_.memData),
                                      0,
                                      seq_.pc);
                seq_.storeStarted = true;
                seq_.phase = SeqState::Phase::MEM_WAIT;
                seq_.note = "Sequential store wait";
                lastSummary_ = seq_.note;
            }
            break;
        }

        seq_.wbValue = evalALU(inst, a, b);
        seq_.phase = SeqState::Phase::WRITEBACK;
        seq_.note = "Sequential ALU";
        lastSummary_ = seq_.note;
        break;
    }

    case SeqState::Phase::MEM_WAIT:
        lastSummary_ = seq_.note.empty() ? "Sequential memory wait" : seq_.note;
        break;

    case SeqState::Phase::WRITEBACK:
        if (seq_.inst.writesRegister())
        {
            const int dest = (seq_.inst.op == Opcode::JAL ? 15 : seq_.inst.rd);
            writeReg(dest, seq_.wbValue);
            updateZeroFlagIfNeeded(seq_.inst, seq_.wbValue);
        }
        pc_ = seq_.pc + 1;
        seq_.loadStarted = false;
        seq_.storeStarted = false;
        seq_.phase = SeqState::Phase::IDLE;
        seq_.note = "Sequential writeback";
        lastSummary_ = seq_.note;
        break;

    case SeqState::Phase::HALTED:
        lastSummary_ = "HALTED";
        break;
    }

    ++cycles_;
    regs_[0] = 0;
}

std::string Simulator::step()
{
    if (halted_)
        return faulted_ ? "FAULT" : "HALTED";

    switch (mode_)
    {
    case ExecMode::NO_PIPE_NO_CACHE:
        stepSequentialCycle(false);
        break;
    case ExecMode::NO_PIPE_CACHE:
        stepSequentialCycle(true);
        break;
    case ExecMode::PIPE_NO_CACHE:
        stepPipelineCycle(false);
        break;
    case ExecMode::PIPE_CACHE:
        stepPipelineCycle(true);
        break;
    }

    if (faulted_)
        return "FAULT";
    return halted_ ? "HALTED" : "OK";
}

std::string Simulator::run(uint64_t maxCycles)
{
    uint64_t steps = 0;

    while (!halted_ && !faulted_ && steps < maxCycles)
    {
        const std::string result = step();
        ++steps;

        if (result == "HALTED")
        {
            return "HALTED";
        }

        if (result == "FAULT")
        {
            return faultMessage_.empty() ? "FAULT" : ("FAULT: " + faultMessage_);
        }
    }

    if (halted_)
    {
        return "HALTED";
    }

    if (faulted_)
    {
        return faultMessage_.empty() ? "FAULT" : ("FAULT: " + faultMessage_);
    }

    lastSummary_ = "Max cycle limit reached before HALT";
    lastFlags_ = "LIMIT";

    return "Max cycle limit reached before HALT. Increase maxCycles for large benchmarks.";
}

std::string Simulator::runUntilBreakpoint(uint64_t maxCycles)
{
    uint64_t steps = 0;

    while (!halted_ && !faulted_ && steps < maxCycles)
    {
        if (breakpoints_.count(pc_) > 0 && steps > 0)
        {
            lastSummary_ = "Breakpoint reached at PC=" + std::to_string(pc_);
            lastFlags_ = "BREAKPOINT";
            return lastSummary_;
        }

        const std::string result = step();
        ++steps;

        if (result == "HALTED")
        {
            return "HALTED";
        }

        if (result == "FAULT")
        {
            return faultMessage_.empty() ? "FAULT" : ("FAULT: " + faultMessage_);
        }
    }

    if (halted_)
    {
        return "HALTED";
    }

    if (faulted_)
    {
        return faultMessage_.empty() ? "FAULT" : ("FAULT: " + faultMessage_);
    }

    lastSummary_ = "Max cycle limit reached before breakpoint/HALT";
    lastFlags_ = "LIMIT";

    return "Max cycle limit reached before breakpoint/HALT.";
}

std::string Simulator::pipeToString(const PipeReg &p) const
{
    if (!p.valid)
        return "<empty>";

    std::ostringstream oss;
    oss << "PC=" << p.pc << " " << p.inst.toString();
    if (!p.note.empty())
        oss << " " << p.note;
    return oss.str();
}

std::string Simulator::seqPhaseString() const
{
    switch (seq_.phase)
    {
    case SeqState::Phase::IDLE:
        return "IDLE";
    case SeqState::Phase::FETCH_WAIT:
        return "FETCH_WAIT";
    case SeqState::Phase::EXECUTE:
        return "EXECUTE";
    case SeqState::Phase::MEM_WAIT:
        return "MEM_WAIT";
    case SeqState::Phase::WRITEBACK:
        return "WRITEBACK";
    case SeqState::Phase::HALTED:
        return "HALTED";
    }
    return "UNKNOWN";
}

std::string Simulator::hierarchyString() const
{
    if (!hierarchyPort_.busy)
        return "Hierarchy idle";

    std::ostringstream oss;
    oss << "Hierarchy busy: addr=" << hierarchyPort_.address
        << " remaining=" << hierarchyPort_.remaining;
    return oss.str();
}

SimulatorSnapshot Simulator::getSnapshot(uint32_t memStart, uint32_t memLines) const
{
    SimulatorSnapshot s;
    s.cycles = cycles_;
    s.pc = pc_;
    s.halted = halted_;
    s.haltRequested = haltRequested_;
    s.faulted = faulted_;
    s.zFlag = status_.Z;
    s.mode = modeString();
    s.regs = regs_;

    s.ifStage = pipeToString(IF_);
    s.idStage = pipeToString(ID_);
    s.exStage = pipeToString(EX_);
    s.memStage = pipeToString(MEM_);
    s.wbStage = pipeToString(WB_);

    s.summary = faulted_ ? faultMessage_ : lastSummary_;
    s.flags = faulted_ ? "FAULT" : lastFlags_;
    s.hierarchyState = hierarchyString();
    s.seqState = "SEQ phase: " + seqPhaseString();

    if (hierarchyPort_.busy &&
        hierarchyPort_.owner == Stage::IF_STAGE &&
        hierarchyPort_.kind == HierarchyPort::Kind::FETCH)
    {
        s.fetchInFlight = true;
        s.fetchPC = hierarchyPort_.address;
        s.fetchRemaining = hierarchyPort_.remaining;
    }

    if (cachesEnabledForCurrentMode())
    {
        s.l1Hits = l1_.hits();
        s.l1Misses = l1_.misses();
        s.l2Hits = l2_.hits();
        s.l2Misses = l2_.misses();

        for (uint32_t i = 0; i < L1_NUM_LINES; ++i)
        {
            SnapshotCacheRow row;
            row.index = i;
            const auto *line = l1_.getLine(i);
            if (line)
            {
                row.valid = line->valid;
                row.dirty = line->dirty;
                row.tag = line->tag;
                row.data = line->data.words;
            }
            s.l1Rows.push_back(row);
        }

        for (uint32_t i = 0; i < L2_NUM_LINES; ++i)
        {
            SnapshotCacheRow row;
            row.index = i;
            const auto *line = l2_.getLine(i);
            if (line)
            {
                row.valid = line->valid;
                row.dirty = line->dirty;
                row.tag = line->tag;
                row.data = line->data.words;
            }
            s.l2Rows.push_back(row);
        }
    }
    else
    {
        s.l1Hits = 0;
        s.l1Misses = 0;
        s.l2Hits = 0;
        s.l2Misses = 0;

        for (uint32_t i = 0; i < L1_NUM_LINES; ++i)
        {
            SnapshotCacheRow row;
            row.index = i;
            s.l1Rows.push_back(row);
        }

        for (uint32_t i = 0; i < L2_NUM_LINES; ++i)
        {
            SnapshotCacheRow row;
            row.index = i;
            s.l2Rows.push_back(row);
        }
    }

    for (uint32_t i = 0; i < memLines; ++i)
    {
        MemoryRow row;
        row.baseAddress = memStart + i * WORDS_PER_LINE;
        for (uint32_t j = 0; j < WORDS_PER_LINE; ++j)
        {
            row.data[j] = memory_.peekWord(row.baseAddress + j);
        }
        s.memoryRows.push_back(row);
    }

    return s;
}

std::string Simulator::dumpRegs() const
{
    return UI::formatRegisters(getSnapshot());
}

std::string Simulator::dumpPipeline() const
{
    return UI::formatPipeline(getSnapshot());
}

std::string Simulator::dumpMemoryRange(Address start, Address count) const
{
    return UI::formatMemory(getSnapshot(start, (count + WORDS_PER_LINE - 1) / WORDS_PER_LINE).memoryRows);
}

std::string Simulator::dumpCacheL1() const
{
    return UI::formatCacheRows(getSnapshot().l1Rows, "L1 Cache");
}

std::string Simulator::dumpCacheL2() const
{
    return UI::formatCacheRows(getSnapshot().l2Rows, "L2 Cache");
}

std::string Simulator::handleStatus() const
{
    return UI::formatSnapshot(getSnapshot());
}