

#include <s2e/ConfigFile.h>
#include <s2e/S2E.h>   // s2e()
#include <s2e/Utils.h> //hexval

#include "DmaEthMonitor.h"
namespace s2e {
namespace plugins {
namespace hw {

S2E_DEFINE_PLUGIN(DmaEthMonitor, "Monitor DMA_ETH in s2e", "DmaEthMonitor", "PeripheralModelLearning");

namespace {
class DmaEthMonitorState : public PluginState {
public:
    static PluginState *factory(Plugin *, S2EExecutionState *) {
        return new DmaEthMonitorState();
    }

    DmaEthMonitorState *clone() const {
        return new DmaEthMonitorState(*this);
    }
    void set_pll_on() {
        is_pll_on = true;
    }
    void set_pll_off() {
        is_pll_on = false;
    }
    bool get_pll() {
        return is_pll_on;
    }
    void set_odr_on() {
        is_odr_on = true;
    }
    void set_odr_off() {
        is_odr_on = false;
    }
    bool get_odr() {
        return is_odr_on;
    }
    void set_ODSWRDY_on() {
        is_ODSWRDY_on = true;
    }
    void set_ODSWRDY_off() {
        is_ODSWRDY_on = false;
    }
    bool get_ODSWRDY() {
        return is_ODSWRDY_on;
    }
    void set_CFGR_SW_val(uint64_t val);
    uint32_t get_CFGR_SW_val() {
        return CFGR_SW_val;
    }
    bool get_RegState(RegState rs) {
        return RegStateArray[rs];
    }
    void set_RegState(RegState rs, bool b) {
        RegStateArray[rs] = b;
    }
    void set_RxDesc(uint64_t val) {
        RxDesc = (uint32_t) val;
    }
    uint32_t get_RxDesc() {
        return RxDesc;
    }
    std::vector<uint64_t> RxDescAddrv;
    std::vector<uint32_t> EthBufAddr;
    std::vector<uint32_t> EthBufLen;
    void set_MACMIIDR(uint64_t val) {
        MACMIIDR_value = (uint16_t) val;
    }
    uint16_t get_MACMIIDR() {
        return MACMIIDR_value;
    }
    uint32_t get_FL() {
        return FL;
    }
    void set_FL(uint32_t val) {
        FL = val;
    }

private:
    bool is_pll_on = false;
    bool is_odr_on = false;
    bool is_ODSWRDY_on = false;
    uint32_t CFGR_SW_val = 0;
    bool RegStateArray[RegState::RS_END];
    uint32_t RxDesc = 0;
    uint16_t MACMIIDR_value = 0;
    uint32_t FL = 0; // frame length for RxDesc
};

void DmaEthMonitorState::set_CFGR_SW_val(uint64_t val) {
    CFGR_SW_val = (uint32_t) val & 0x3; // get SW value from RCC_CFGR
}
} // namespace

void DmaEthMonitor::initialize() {

    enable_fuzzing = s2e()->getConfig()->getBool(getConfigKey() + ".useFuzzer", false);

    onPeripheralModelLearningConnection = s2e()->getPlugin<PeripheralModelLearning>();
    onPeripheralModelLearningConnection->onSymbWriteEvent.connect(sigc::mem_fun(*this, &DmaEthMonitor::onSymbWrite));
    onPeripheralModelLearningConnection->onSymbReadEvent.connect(sigc::mem_fun(*this, &DmaEthMonitor::onSymbRead));
    s2e()->getCorePlugin()->onTranslateInstructionStart.connect(sigc::mem_fun(*this, &DmaEthMonitor::onTranslateInst));
}

// set the value to in the address with the start and the length
// the value is the actual number start from the start bit which should not be greater than 2**length
bool DmaEthMonitor::set_reg_value(S2EExecutionState *state, uint64_t address, uint32_t value, uint32_t start,
                                  uint32_t length) {
    if (start + length > 32) {
        return false;
    }
    uint32_t Val = get_reg_value(state, address);
    uint32_t msk = (uint32_t) -1 >> start << (32 - length) >> (32 - start - length);
    Val = (Val & ~msk) | (value << start);
    getInfoStream(state) << "set_reg_value " << hexval(address) << " value " << hexval(Val) << "\n";
    return state->mem()->write(address, &Val, sizeof(Val), AddressType::PhysicalAddress);
}

uint32_t DmaEthMonitor::get_reg_value(S2EExecutionState *state, uint64_t address) {
    uint32_t value = 0;
    bool ok = state->mem()->read(address, &value, sizeof(value), AddressType::PhysicalAddress);
    getInfoStream(state) << "Read " << (ok ? " Success" : " Failed") << "\n";
    getInfoStream(state) << "get_reg_value " << hexval(address) << " value " << hexval(value) << "\n";
    return value;
}

void DmaEthMonitor::onSymbWrite(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address,
                                unsigned size, uint64_t concreteValue, void *opaque) {
    DECLARE_PLUGINSTATE(DmaEthMonitorState, state);

    // test
    getWarningsStream(state) << "write to address " << hexval(address) << " value " << hexval(concreteValue) << "\n";
    // PLL on
    if (address == 0x42470060 && concreteValue & 0x1) {
        getWarningsStream(state) << " set pll on "
                                 << " value " << hexval(concreteValue) << "\n";
        plgState->set_RegState(RegState::PLL, true);
    } else if (address == 0x420E0040 && concreteValue & 0x1) {
        getWarningsStream(state) << " set odr on physaddress " << hexval(address) << " value " << hexval(concreteValue)
                                 << "\n";
        plgState->set_RegState(RegState::ODR, true);
    } else if (address == 0x420E0044 && concreteValue & 0x1) {
        getWarningsStream(state) << " set odswrdy on physaddress " << hexval(address) << " value "
                                 << hexval(concreteValue) << "\n";
        plgState->set_RegState(RegState::ODSWRDY, true);
    } else if (address == RCC_CFGR) { // set SW in CFGR
        getWarningsStream(state) << " store the CFGR value : " << hexval(concreteValue) << " pc "
                                 << hexval(state->regs()->getPc()) << "\n";
        plgState->set_CFGR_SW_val(concreteValue);
    } else if (address == ETH_DMABMR) {
        if (concreteValue & 0x1) { // SR
            getWarningsStream(state) << " set the SR in DMABMR: " << hexval(concreteValue) << " pc "
                                     << hexval(state->regs()->getPc()) << "\n";
            plgState->set_RegState(RegState::DMABMR_SR, true);
        }
    } else if (address == ETH_DMARDLAR) {
        getWarningsStream(state) << " get the RxDesc : address " << hexval(concreteValue) << " pc "
                                 << hexval(state->regs()->getPc()) << "\n";
        plgState->set_RegState(RegState::SRL, true);
        plgState->set_RxDesc(concreteValue);
        for (int i = 0; i < 28; i++) {
            getWarningsStream(state) << " get the addr[" << i
                                     << "] val: " << hexval(get_reg_value(state, concreteValue + i * 4)) << "\n";
        }
        if(g_s2e_cache_mode & enable_fuzzing )
            onDmaEthFuzz.emit(state, concreteValue);
        else  // should maintain the RxDesc to ensure the app run to main loop
            read_prepare_RxDesc(state,concreteValue);
        for (int i = 0; i < 28; i++) {
            getWarningsStream(state) << " get the addr[" << i
                                     << "] val: " << hexval(get_reg_value(state, concreteValue + i * 4)) << "\n";
        }

    } else if (address == ETH_MACMIIAR) {
        if (concreteValue & 0x1) { // MB
            getWarningsStream(state) << " set the MB in MACMIIAR: " << hexval(concreteValue) << " pc "
                                     << hexval(state->regs()->getPc()) << "\n";
            plgState->set_RegState(RegState::MACMIIAR_MB, true);
        }
        if (concreteValue & (uint64_t) PHYReg::PHY_BSR) {
            getWarningsStream(state) << " set the PHY_BSR in MACMIIAR: " << hexval(concreteValue) << " pc "
                                     << hexval(state->regs()->getPc()) << "\n";
            plgState->set_RegState(RegState::MACMIIAR_PHY_BSR, true);
        }
        if (concreteValue & 0x8000) { // DP83848_BCR_SR
            getWarningsStream(state) << " set the SR in MACMIIAR: " << hexval(concreteValue) << " pc "
                                     << hexval(state->regs()->getPc()) << "\n";
            plgState->set_RegState(RegState::MACMIIAR_DP83848_BCR_SR, true);
        }
    }
}

void DmaEthMonitor::onSymbRead(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address,
                               unsigned size, uint64_t concreteValue, void *opaque, uint32_t *return_value) {

    DECLARE_PLUGINSTATE(DmaEthMonitorState, state);

    if (address == RCC_CR) {
        if (concreteValue & 0x10000) { // when HSE on read HSE need to be HSE rdy
            *return_value = concreteValue | HSERDY;
            getWarningsStream(state) << "HSE RDY when HSE on " << *return_value << "\n";
        }
        if (plgState->get_RegState(RegState::PLL)) {
            *return_value = concreteValue | PLLRDY;
            getWarningsStream(state) << "PLL RDY when PLL on" << *return_value << "\n";
        }
    } else if (address == 0x40007004 &&
               (plgState->get_RegState(RegState::ODR) || plgState->get_RegState(RegState::ODSWRDY))) {
        *return_value = concreteValue | 0x30000;
        getWarningsStream(state) << "ODRDY&ODSWRDY when on" << *return_value << "\n";
    } else if (address == RCC_CFGR) { // get SW in CFGR
        *return_value = concreteValue | (plgState->get_CFGR_SW_val() << 2);
        getWarningsStream(state) << " SWS is same to SW in CFGR: value " << *return_value << "\n";
    } else if (address == ETH_DMABMR) {
        // TODO : maybe should change the value in onSymbWrite but not to return the fake value in every LD
        // the problem here is that when the SR set it should be all set to 0 , and the DMABMR whill be use for other
        // func so the Reset func can not be actually did here. Fortunatelly , code just read the first value not all
        // the value in the REG
        if (plgState->get_RegState(RegState::DMABMR_SR)) { // SR is set;
            *return_value = concreteValue & ((uint64_t) -2);
            getWarningsStream(state) << " return the value with clear the SR in DMABMR: " << *return_value << "\n";
        }
    } else if (address == ETH_MACMIIAR) {
        if (plgState->get_RegState(RegState::MACMIIAR_MB)) {
            *return_value = concreteValue & ((uint64_t) -2);
            getWarningsStream(state) << " return the value with clear the MB in MACMIIAR: " << *return_value << " pc "
                                     << hexval(state->regs()->getPc()) << "\n";
        }
        // *return_value = concreteValue & ~0x8000;
        // getWarningsStream(state) << " return with clear the SR in MACMIIAR: " << *return_value << " pc " <<
        // hexval(state->regs()->getPc()) <<  "\n"; if(plgState->get_RegState(RegState::MACMIIAR_DP83848_BCR_SR)) {
        //     *return_value = concreteValue & ~0x8000;
        //     getWarningsStream(state) << " return the value with clear the SR in MACMIIAR: " << *return_value << "\n";
        // }
    } else if (address == ETH_MACMIIDR) {
        if (plgState->get_RegState(RegState::MACMIIAR_PHY_BSR)) {
            *return_value = concreteValue | (PHY_LINKED_STATUS | PHY_AUTONEGO_COMPLETE);
            getWarningsStream(state) << " return in MACMIIDR : " << *return_value << " pc "
                                     << hexval(state->regs()->getPc()) << "\n";
        }
    } else if (address == ETH_DMARDLAR) {
        if (plgState->get_RegState(RegState::SRL)) {
            getWarningsStream(state) << "get RxDesc list address" << concreteValue << " pc "
                                     << hexval(state->regs()->getPc()) << "\n";
            // prepare_frame(state,concreteValue);
        }
    } else if(address == ETH_DMASR ){
        getWarningsStream(state) << "read DMASR and emit the Fuzz pc " << hexval(state->regs()->getPc()) << "\n";
        if(enable_fuzzing & g_s2e_cache_mode)
            onDmaEthFuzz.emit(state,plgState->get_RxDesc());
        else 
            read_prepare_RxDesc(state, plgState->get_RxDesc());
    } else {
        int i;
        std::vector<uint64_t> v = plgState->RxDescAddrv;
        for (i = 0; i < v.size(); i++) {
            if (address == v[i] && concreteValue & 0x80000000) {
                *return_value = concreteValue & ~0x80000000;
                getWarningsStream(state) << " return without own in RxDesc: " << *return_value << " pc "
                                         << hexval(state->regs()->getPc()) << "\n";
            }
        }
    }
}

void DmaEthMonitor::read_prepare_RxDesc(S2EExecutionState *state, uint64_t address) {
    DECLARE_PLUGINSTATE(DmaEthMonitorState, state);
    uint64_t RxDescAddr = address;
    uint64_t NextAddr;
    uint32_t DESC1 = 0;
    uint32_t i = 0;

    while (1) {
        /* Prepare for the application to read */
        set_reg_value(state, RxDescAddr, 0, ETH_DMARXDESC_OWN, 1); // clear the OWN bit
        if (RxDescAddr == address) {                               // the first descriptor
            set_reg_value(state, RxDescAddr, 1, ETH_DMARXDESC_FS, 1);
        }

        /* Read from the Descriptor */
        DESC1 = get_reg_value(state, RxDescAddr + 4);
        plgState->EthBufLen.emplace_back(DESC1 & 0x1FFF);
        plgState->EthBufAddr.emplace_back(get_reg_value(state, RxDescAddr + 8));
        plgState->RxDescAddrv.emplace_back(RxDescAddr);
        getWarningsStream(state) << " get the descriptor[" << i << "] address " << hexval(RxDescAddr) << "\n";
        NextAddr = get_reg_value(state, RxDescAddr + 12);
        if (NextAddr == address) { // the last descriptor
            set_reg_value(state, RxDescAddr, 1, ETH_DMARXDESC_LS, 1);
            break;
        }

        RxDescAddr = NextAddr;

        // LOG OUT
        getWarningsStream(state) << " get the EthBuf [" << i << "] address "
                                 << hexval(get_reg_value(state, RxDescAddr + 8)) << " length " << hexval(DESC1 & 0x1FFF)
                                 << "\n";
        i++;
    }

    getWarningsStream(state) << i << " Descriptors have been found"
                             << "\n";

    /* ------   Test   ------- */
    // Prepare Payload for Application to read
    // TODO: add CRC
    char a[] = "crc.In some cases, using your fingerprint to log into the system may inhibit certain other functions "
               "of the desktop environment. For example, ";
    state->mem()->write(plgState->EthBufLen[0], a, sizeof(a), s2e::PhysicalAddress);
    plgState->set_FL(sizeof(a));
    set_reg_value(state, RxDescAddr, sizeof(a), 16, 14); // set the FrameLen in the last descriptor
    getWarningsStream(state) << "set the frame for RxDesc: address " << hexval(plgState->EthBufAddr[0]) << " length "
                             << plgState->get_FL() << "\n";
}

void DmaEthMonitor::onTranslateInst(ExecutionSignal *signal, S2EExecutionState *state, TranslationBlock *tb,
                                    uint64_t pc) {
    if (pc == 0x9001B3E) { // lwip_init()
        char func_name[] = "ethernetif_input";
        s2e()->getWarningsStream() << "check the func of exit : " << func_name << " address " << hexval(pc) << "\n";
        signal->connect(sigc::mem_fun(*this, &DmaEthMonitor::onInstExec));
    }
}

void DmaEthMonitor::onInstExec(S2EExecutionState *state, uint64_t pc) {
    s2e()->getWarningsStream() << "time to  exit : " << hexval(pc) << "\n";
    exit(0);
}

} // namespace hw
} // namespace plugins
} // namespace s2e
