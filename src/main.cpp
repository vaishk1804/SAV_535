#include "Simulator.h"
#include "UI.h"
#include <iostream>
#include <sstream>
#include <string>

int main()
{
    Simulator sim;

    std::cout << "SAV_535 simulator CLI\n";
    std::cout << "Commands: LOADASM <file>, MODE <0|1|2|3>, STEP, RUN [n], RUNBP [n], "
                 "BP <addr>, CLEARBP <addr>, LISTBP, CLEARBPS, "
                 "REGS, PIPE, CACHE1, CACHE2, MEM <start> <count>, STATE, RESET, HELP, QUIT\n";

    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line))
    {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd.empty())
            continue;

        if (cmd == "QUIT" || cmd == "EXIT")
        {
            break;
        }
        else if (cmd == "HELP")
        {
            std::cout
                << "LOADASM file | MODE 0..3 | STEP | RUN [cycles] | RUNBP [cycles] | "
                << "BP addr | CLEARBP addr | LISTBP | CLEARBPS | "
                << "REGS | PIPE | CACHE1 | CACHE2 | MEM start count | STATE | RESET | QUIT\n";
        }
        else if (cmd == "LOADASM")
        {
            std::string file;
            iss >> file;
            try
            {
                std::cout << sim.loadProgramAsm(file) << "\n";
            }
            catch (const std::exception &e)
            {
                std::cout << "Error: " << e.what() << "\n";
            }
        }
        else if (cmd == "MODE")
        {
            int m = 3;
            iss >> m;
            if (m < 0 || m > 3)
            {
                std::cout << "Invalid mode. Use 0, 1, 2, or 3.\n";
            }
            else
            {
                sim.setMode(static_cast<ExecMode>(m));
                std::cout << "Mode set\n";
            }
        }
        else if (cmd == "STEP")
        {
            std::cout << sim.step() << "\n";
            std::cout << sim.handleStatus();
        }
        else if (cmd == "RUN")
        {
            uint64_t n = 700000;
            iss >> n;
            std::cout << sim.run(n) << "\n";
            std::cout << sim.handleStatus();
        }
        else if (cmd == "RUNBP")
        {
            uint64_t n = 700000;
            iss >> n;
            std::cout << sim.runUntilBreakpoint(n) << "\n";
            std::cout << sim.handleStatus();
        }
        else if (cmd == "BP")
        {
            uint32_t addr = 0;
            if (iss >> addr)
            {
                sim.addBreakpoint(addr);
                std::cout << "Breakpoint added at " << addr << "\n";
            }
            else
            {
                std::cout << "Usage: BP <addr>\n";
            }
        }
        else if (cmd == "CLEARBP")
        {
            uint32_t addr = 0;
            if (iss >> addr)
            {
                sim.clearBreakpoint(addr);
                std::cout << "Breakpoint cleared at " << addr << "\n";
            }
            else
            {
                std::cout << "Usage: CLEARBP <addr>\n";
            }
        }
        else if (cmd == "LISTBP")
        {
            auto bps = sim.listBreakpoints();
            if (bps.empty())
            {
                std::cout << "No breakpoints set\n";
            }
            else
            {
                std::cout << "Breakpoints:";
                for (auto bp : bps)
                {
                    std::cout << " " << bp;
                }
                std::cout << "\n";
            }
        }
        else if (cmd == "CLEARBPS")
        {
            sim.clearBreakpoints();
            std::cout << "All breakpoints cleared\n";
        }
        else if (cmd == "REGS")
        {
            std::cout << sim.dumpRegs();
        }
        else if (cmd == "PIPE")
        {
            std::cout << sim.dumpPipeline();
        }
        else if (cmd == "CACHE1")
        {
            std::cout << sim.dumpCacheL1();
        }
        else if (cmd == "CACHE2")
        {
            std::cout << sim.dumpCacheL2();
        }
        else if (cmd == "MEM")
        {
            uint32_t start = 0, count = 16;
            iss >> start >> count;
            std::cout << sim.dumpMemoryRange(start, count);
        }
        else if (cmd == "STATE")
        {
            std::cout << sim.handleStatus();
        }
        else if (cmd == "RESET")
        {
            sim.reset();
            std::cout << "Reset\n";
        }
        else
        {
            std::cout << "Unknown command\n";
        }
    }

    return 0;
}