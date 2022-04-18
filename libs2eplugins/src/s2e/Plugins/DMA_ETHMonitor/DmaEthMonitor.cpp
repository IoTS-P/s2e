

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

    getInfoStream() << " --- Initialize the register for DMA ETH --- " << "\n";
    // set RCC HSERDY 
    set_reg_value(g_s2e_state, 0x40023800, 0x20000);
    // set RCC CFG
    set_reg_value(g_s2e_state, 0x40023808, 0x8);
    // set PWR ODRDY & ODSWRDY
    set_reg_value(g_s2e_state, 0x40007004, 0x30000);

}

bool DmaEthMonitor::set_reg_value(S2EExecutionState *state,uint64_t address, uint32_t value, uint32_t start, uint32_t length) {
    if( start+length > 32 )  {
        return false;
    }
    uint32_t Val = get_reg_value(state, address);
    uint32_t msk = (uint32_t) -1  >> start << (32 - length) >> (32 - start - length) ;
    Val =  ( Val & ~msk ) | value;
    getInfoStream(state) << "set_reg_value " << hexval(address) << " value " << hexval(Val)  << "\n";
    return state->mem()->write(address, &Val, sizeof(Val), AddressType::PhysicalAddress);
}


uint32_t DmaEthMonitor::get_reg_value(S2EExecutionState *state,uint64_t address){
    uint32_t value =0;
    bool ok = state->mem()->read(address, &value, sizeof(value), AddressType::PhysicalAddress);
    getInfoStream(state) << "Read " << (ok ? " Success" : " Failed") << "\n";
    getInfoStream(state) << "get_reg_value " << hexval(address) << " value " << hexval(value) << "\n";
    return value;
}

void DmaEthMonitor::onSymbWrite(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                     uint64_t concreteValue, void *opaque) {
    
    bool ok = true;
    // Change invoked by conditions 
    if( address == 0x40023804) { //ETH_MACFFR
        ok &= set_reg_value(state, 0x40023800, 0x1, 25, 1);
    }else if( address == 0x40029000 && concreteValue&0x1) { // set ETH_MAC_MABMR
        ok &= set_reg_value(state, address, 0);
    } else if (address == 0x40028010 && concreteValue&0x1) { // MACMIIAR
        ok &= set_reg_value(state, address, concreteValue&~0x1);
    } else if (address == 0x400380C) { // receive descriptor list address
        getInfoStream(g_s2e_state) << "Get RxDesc Address " << concreteValue <<  "\n";
        Prepare_Rx_Desc(state, concreteValue);
        read_from_RxDesc(state, concreteValue);
    }                

}

// prepare a read descriptor for LwIP_TCP_Echo_Read
void DmaEthMonitor::Prepare_Rx_Desc(S2EExecutionState *state, uint64_t address) {
    // ETH_DMADescTypeDef *dmarxdesc = (ETH_DMADescTypeDef *)address;
    uint64_t DescAddr = address;
    uint32_t descidx = 0;
    uint32_t length = 1000;
    // uint8_t rxdataready = 0U;
    uint32_t pbufAddr = 0x50000000; // 获得一个空的地址
    for(;descidx<4;descidx++)
    {
        length = (descidx = 3) ? 48 : 1000;

        // clear own bit
        set_reg_value(state, DescAddr, 0, ETH_DMARXDESC_OWN, 1);
        if (descidx ==0) {
            set_reg_value(state, DescAddr, 1, ETH_DMARXDESC_FS, 1); // set firt desc
        }
        if (descidx == 3) {
            set_reg_value(state, DescAddr, 1, ETH_DMARXDESC_LS, 1); // set last descriptor 
            set_reg_value(state, DescAddr, length, ETH_DMARXDESC_FL , 14); // set length
        }

        set_reg_value(state, DescAddr+2, pbufAddr); // 设置pbuffer 地址到 DescAddr->DESC2
        for(uint32_t i=0;i<length;i+=4,pbufAddr+=4){
            set_reg_value(state, (uint64_t)pbufAddr, i);
        }

        DescAddr = (uint64_t)get_reg_value(state,DescAddr+3); // DesAddr = DesAddr->DESC3
    }
}

void DmaEthMonitor::read_from_RxDesc(S2EExecutionState *state, uint64_t address)
{
    uint64_t RxDescAddr = address;
    uint32_t DESC0 = get_reg_value(state, RxDescAddr);
    uint32_t i;
    for(i=0;!(DESC0 >> ETH_DMARXDESC_LS & 0x1) && i<4;i++) {
        DESC0 = get_reg_value(state, RxDescAddr);
        EthBufAddr.emplace_back(get_reg_value(state, RxDescAddr+2));
        // BufLen[i] = DESC0 >> ETH_DMARXDESC_FL & 0x3FFF;
        EthBufLen.emplace_back(DESC0 >> ETH_DMARXDESC_FL & 0x3FFF);
        // BufAddr[i] = get_reg_value(state, RxDescAddr+2);
        RxDescAddr = (uint64_t)get_reg_value(state, RxDescAddr+3);
    }

    // TEST_OUT
    if(!EthBufAddr.empty())
    {
        auto i = EthBufAddr.begin();
        auto j = EthBufLen.begin();
        for(;i!=EthBufAddr.end() && j!=EthBufAddr.end();i++,j++)
        {
            getWarningsStream(state) << "read_from_RxDesc: DmaEthBufAddr " << hexval(*i) << " Length " << hexval(*j) << "\n"; 
        }
    }
}


} // namespace hw
} // namespace plugins
} // namespace s2e
