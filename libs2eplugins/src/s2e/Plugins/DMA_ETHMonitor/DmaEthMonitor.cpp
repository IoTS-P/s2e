

#include <s2e/Utils.h> //hexval 
#include <s2e/ConfigFile.h>
#include <s2e/S2E.h> // s2e()

#include "DmaEthMonitor.h"

namespace s2e {
namespace plugins {
namespace hw {


S2E_DEFINE_PLUGIN(DmaEthMonitor,"Monitor DMA_ETH in s2e", "DmaEthMonitor", "PeripheralModelLearning");


void DmaEthMonitor::initialize() {
    

    onPeripheralModelLearningConnection = s2e()->getPlugin<PeripheralModelLearning>();
    onPeripheralModelLearningConnection->onSymbWriteEvent.connect(sigc::mem_fun(*this, &DmaEthMonitor::onSymbWrite));
    // onPeripheralModelLearningConnection->onSymbReadEvent.connect(sigc::mem_fun(*this, &DmaEthMonitor::onSymbRead));

    /* Initialize the register for DMA ETH */
    // set RCC HSERDY 
    set_reg_value(g_s2e_state, 0x40023800, 0x20000);
    // set RCC CFG
    set_reg_value(g_s2e_state, 0x40023808, 0x8);
    // set PWR ODRDY & ODSWRDY
    set_reg_value(g_s2e_state, 0x40007004, 0x30000);

}

bool DmaEthMonitor::set_reg_value(S2EExecutionState *state,uint64_t address, uint32_t value, uint32_t start, uint32_t length) {
    if( start+length >= 32 )  {
        return false;
    }
    uint32_t Val = get_reg_value(state, address);
    uint32_t msk = (uint32_t) -1  >> start << (32 - length) >> (32 - start - length) ;
    Val =  ( Val & ~msk ) | value;
    getInfoStream() << "set_reg_value " << hexval(address) << " value " << hexval(value)  << "\n";
    return state->mem()->write(address, &Val, sizeof(Val));
}

uint32_t DmaEthMonitor::get_reg_value(S2EExecutionState *state,uint64_t address){
    uint32_t value =0;
    bool ok = state->mem()->read(address, &value, sizeof(value));
    getInfoStream() << ok << "\n";
    getInfoStream() << "phaddr " << hexval(address) << " value " << hexval(value) << "\n";
    return value;
}

void DmaEthMonitor::onSymbWrite(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                     uint64_t concreteValue, void *opaque) {
    
    bool ok = true;
    // Change invoked by conditions 
    if( address == 0x40023804) {
        ok &= set_reg_value(state, 0x40023800, 0x1, 25, 1);
    }else if( address == 0x40029000 && concreteValue&0x1) { // set ETH_MAC_MABMR
        ok &= set_reg_value(state, address, 0);
    } else if (address == 0x40028010 && concreteValue&0x1) { // MACMIIAR
        ok &= set_reg_value(state, address, concreteValue&~0x1);
    }                  

}



} // namespace hw
} // namespace plugins
} // namespace s2e
