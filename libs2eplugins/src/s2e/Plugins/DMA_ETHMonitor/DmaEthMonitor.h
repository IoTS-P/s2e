#ifndef S2E_PLUGINS_DmaEthMonitor_H
#define S2E_PLUGINS_DmaEthMonitor_H

#include <s2e/CorePlugin.h>
#include <s2e/Plugin.h>
#include <s2e/Plugins/uEmu/PeripheralModelLearning.h>
#include <s2e/S2EExecutionState.h>    // S2ExecutionState
#include <s2e/SymbolicHardwareHook.h> // SymbolicHardwareAccessType

#include <map>
#include <vector>

namespace s2e {
namespace plugins {
namespace hw {

#define ETH_RX_DESC_CNT 4U
#define ETH_DMARXDESC_OWN 31
#define ETH_DMARXDESC_FS 9
#define ETH_DMARXDESC_LS 8
#define ETH_DMARXDESC_FL 16
#define PHY_LINKED_STATUS ((uint16_t) 0x0004)     /*!< Valid link established               */
#define PHY_AUTONEGO_COMPLETE ((uint16_t) 0x0020) /*!< Auto-Negotiation process completed   */
#define HSERDY 0x20000
#define PLLRDY 0x2000000

/* ----------- Register Addresses ---------*/
#define PWR_CSR 0x40007004
#define RCC_CR 0x40023800
#define RCC_PLLCFGR 0x40023804
#define RCC_CFGR 0x40023808
#define ETH_MACMIIAR 0x40028010
#define ETH_MACMIIDR 0x40028014
#define ETH_DMABMR 0x40029000
#define ETH_DMARDLAR 0x4002900C
#define ETH_DMASR 0x40029014

#define vec \
    { PWR_CSR, RCC_CR, RCC_CFGR, ETH_MACMIIAR, ETH_MACMIIDR, ETH_DMABMR, ETH_DMARDLAR }

enum RegState {
    PLL,
    ODR,
    ODSWRDY,
    DMABMR_SR,
    MACMIIAR_MB,
    MACMIIAR_DP83848_BCR_SR,
    MACMIIAR_PHY_BSR,
    SRL,    // start of receive list
    RS_END, // the last one of RegState
};

// REG value for MACMIIAR MII Register [10:6]
enum class PHYReg : uint64_t {
    PHY_BSR = 0x1 << 6,
};
struct RegVal {
    uint64_t RegAddr;
    uint64_t RegBit;
};

class DmaEthMonitor : public Plugin {
    S2E_PLUGIN
private:
    PeripheralModelLearning *onPeripheralModelLearningConnection;
    std::map<RegState, RegVal> RegMap;
    bool enable_fuzzing;

public:
    DmaEthMonitor(S2E *s2e) : Plugin(s2e) {
    }

    sigc::signal<void, S2EExecutionState *, uint64_t /* RxDescAddr */> onDmaEthFuzz;
    void initialize();

    bool set_reg_value(S2EExecutionState *state, uint64_t address, uint32_t value, uint32_t start = 0,
                       uint32_t length = 32);

    void onSymbWrite(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                     uint64_t concreteValue, void *);

    void onSymbRead(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                    uint64_t concreteValue, void *, uint32_t *);

    uint32_t get_reg_value(S2EExecutionState *state, uint64_t address);

    void read_prepare_RxDesc(S2EExecutionState *state, uint64_t address);

    void onTranslateInst(ExecutionSignal *signal, S2EExecutionState *state, TranslationBlock *tb, uint64_t pc);
    void onInstExec(S2EExecutionState *state, uint64_t pc);
};
} // namespace hw
} // namespace plugins
} // namespace s2e

#endif
