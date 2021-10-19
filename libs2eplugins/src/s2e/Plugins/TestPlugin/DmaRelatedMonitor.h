#ifndef S2E_PLUGINS_INSTRTRACKER_H
#define S2E_PLUGINS_INSTRTRACKER_H

#include <s2e/Plugin.h>
#include <s2e/CorePlugin.h>
#include <s2e/S2EExecutionState.h>

namespace s2e {
namespace plugins {

class DmaRelatedMonitor : public Plugin
{
    S2E_PLUGIN
    uint64_t m_address;
public:
    DmaRelatedMonitor(S2E *s2e): Plugin(s2e) {}

    void initialize();
    void onTranslateInstruction(ExecutionSignal *signal,
            S2EExecutionState *state,
            TranslationBlock *tb,
            uint64_t pc);
    void onInstructionExecution(S2EExecutionState *state,
            uint64_t pc);

    // sigc::signal<void,
    //             S2EExecutionState *, // The first parameter of the callback is the state
    //             uint64_t             // The second parameter is an integer representing the program counter
    //             > onPeriodicEvent;

};

} // namespace plugins
} // namespace s2e

#endif
