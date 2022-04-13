#ifndef S2E_PLUGINS_DmaEthMonitor_H
#define S2E_PLUGINS_DmaEthMonitor_H

#include <s2e/CorePlugin.h>
#include <s2e/Plugin.h>
#include <s2e/SymbolicHardwareHook.h> // SymbolicHardwareAccessType
#include <s2e/S2EExecutionState.h> // S2ExecutionState
#include <s2e/Plugins/uEmu/PeripheralModelLearning.h>

namespace s2e {
namespace plugins {
namespace hw {

class DmaEthMonitor : public Plugin {
    S2E_PLUGIN
private:
    PeripheralModelLearning *onPeripheralModelLearningConnection;

public:
    DmaEthMonitor(S2E *s2e) : Plugin(s2e) { }

    void initialize();
    void onSymbWrite(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                     uint64_t concreteValue, void *);

    bool set_reg_value(S2EExecutionState *state,uint64_t address, uint32_t value, 
                                        uint32_t start = 0, uint32_t length = 32);
    
                                        
    uint32_t get_reg_value(S2EExecutionState *state, uint64_t address);
};
} // namespace hw
} // namespace plugins
} // namespace s2e

#endif 
