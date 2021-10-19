// From libs2ecore. Provides the hexval function
#include <s2e/Utils.h>
#include <s2e/S2E.h>
#include "DmaRelatedMonitor.h"

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(DmaRelatedMonitor, "Example s2e plugin", "DmaRelatedMonitor",);

class DmaRelatedMonitorState : public PluginState {
private:
    int m_count;

public:
    DmaRelatedMonitorState() {
        m_count = 0;
    }

    virtual ~DmaRelatedMonitorState() {}

    static PluginState *factory(Plugin*, S2EExecutionState*) {
        return new DmaRelatedMonitorState();
    }

    DmaRelatedMonitorState *clone() const {
        return new DmaRelatedMonitorState(*this);
    }

    // void increment() {
    //     ++m_count;
    // }

    // int get() {
    //     return m_count;
    // }
};

void DmaRelatedMonitor::initialize() {
    m_address = (uint64_t) s2e()->getConfig()->getInt(getConfigKey() + ".addressToTrack");

    // This indicates that our plugin is interested in monitoring instruction translation.
    // For this, the plugin registers a callback with the onTranslateInstruction signal.
    s2e()->getCorePlugin()->onTranslateInstructionStart.connect(
        sigc::mem_fun(*this, &DmaRelatedMonitor::onTranslateInstruction));
}

void DmaRelatedMonitor::onTranslateInstruction(ExecutionSignal *signal,
                                                S2EExecutionState *state,
                                                TranslationBlock *tb,
                                                uint64_t pc) {
    if (m_address == pc) {
        // When we find an interesting address, ask S2E to invoke our callback when the address is actually
        // executed
        signal->connect(sigc::mem_fun(*this, &DmaRelatedMonitor::onInstructionExecution));
    }
}

// This callback is called only when the instruction at our address is executed.
// The callback incurs zero overhead for all other instructions
void DmaRelatedMonitor::onInstructionExecution(S2EExecutionState *state, uint64_t pc) {
    s2e()->getWarningsStream() << "Executing instruction at " << hexval(pc) << '\n';
    s2e()->getWarningsStream() << "------------------------------------- " << hexval(pc) << '\n';
    // The plugins can arbitrarily modify/observe the current execution state via the execution state pointer.
    // Plugins can also call the s2e() method to use the S2E API
    // This macro declares the plgState variable of type DmaRelatedMonitorState.
    // It automatically takes care of retrieving the right plugin state attached to the specified execution state
    // DECLARE_PLUGINSTATE(DmaRelatedMonitorState, state);

    // Increment the count
    // plgState->increment();

    // // Trigger the event
    // if ((plgState->get() % 10) == 0) {
    //     onPeriodicEvent.emit(state, pc);
    // }
}

} // namespace plugins 
} // namespace s2e
