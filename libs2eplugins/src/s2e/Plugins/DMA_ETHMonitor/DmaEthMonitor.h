#ifndef S2E_PLUGINS_DmaEthMonitor_H
#define S2E_PLUGINS_DmaEthMonitor_H

#include <s2e/CorePlugin.h>
#include <s2e/Plugin.h>
#include <s2e/SymbolicHardwareHook.h> // SymbolicHardwareAccessType
#include <s2e/S2EExecutionState.h> // S2ExecutionState
#include <s2e/Plugins/uEmu/PeripheralModelLearning.h>

#include <vector>

namespace s2e {
namespace plugins {
namespace hw {

#define ETH_RX_DESC_CNT         4U
#define ETH_DMARXDESC_OWN         31
#define ETH_DMARXDESC_FS          9 
#define ETH_DMARXDESC_LS          9
#define ETH_DMARXDESC_FL          16



class DmaEthMonitor : public Plugin {
    S2E_PLUGIN
private:
    PeripheralModelLearning *onPeripheralModelLearningConnection;
    std::vector<uint32_t> EthBufAddr;
    std::vector<uint32_t> EthBufLen;

public:
    DmaEthMonitor(S2E *s2e) : Plugin(s2e) { }

    void initialize();

    bool set_reg_value(S2EExecutionState *state,uint64_t address, uint32_t value, 
                                        uint32_t start = 0, uint32_t length = 32);
    
    void onSymbWrite(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                     uint64_t concreteValue, void *);
    
    void onSymbRead(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                     uint64_t concreteValue, void *, uint32_t *);
                                        
    uint32_t get_reg_value(S2EExecutionState *state, uint64_t address);

    void Prepare_Rx_Desc(S2EExecutionState *state, uint64_t address);
    
    void read_from_RxDesc(S2EExecutionState *state, uint64_t address);

    void onTranslateInst(ExecutionSignal *signal, S2EExecutionState *state, TranslationBlock *tb, uint64_t pc);
    void onInstExec(S2EExecutionState *state, uint64_t pc );
};
} // namespace hw
} // namespace plugins
} // namespace s2e

#endif 
