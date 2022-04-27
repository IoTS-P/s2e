

#include <s2e/Utils.h> //hexval 
#include <s2e/ConfigFile.h>
#include <s2e/S2E.h> // s2e()

#include "DmaEthMonitor.h"

namespace s2e {
namespace plugins {
namespace hw {


S2E_DEFINE_PLUGIN(DmaEthMonitor,"Monitor DMA_ETH in s2e", "DmaEthMonitor", "PeripheralModelLearning");

namespace {
    class DmaEthMonitorState : public PluginState {
    public:
        static PluginState *factory(Plugin*, S2EExecutionState*) {
            return new DmaEthMonitorState();
        }

        DmaEthMonitorState *clone() const {
            return new DmaEthMonitorState(*this);
        }
        void set_pll_on() { is_pll_on = true; }
        void set_pll_off() { is_pll_on = false;}
        bool get_pll() { return is_pll_on;}
        void set_odr_on() { is_odr_on = true; }
        void set_odr_off() { is_odr_on = false;}
        bool get_odr() { return is_odr_on;}
        void set_ODSWRDY_on() { is_ODSWRDY_on = true; }
        void set_ODSWRDY_off() { is_ODSWRDY_on = false;}
        bool get_ODSWRDY() { return is_ODSWRDY_on;}
        void set_CFGR_SW_val(uint64_t val );
        uint32_t get_CFGR_SW_val(){ return CFGR_SW_val;}
    private:
        bool is_pll_on = false;
        bool is_odr_on = false;
        bool is_ODSWRDY_on = false;
        uint32_t CFGR_SW_val = 0;
    };

    void DmaEthMonitorState::set_CFGR_SW_val(uint64_t val ) {
        // if ( val > 3 ) {
        
        // }
        CFGR_SW_val = (uint32_t) val & 0x3;
    }
}

void DmaEthMonitor::initialize() {
    

    onPeripheralModelLearningConnection = s2e()->getPlugin<PeripheralModelLearning>();
    onPeripheralModelLearningConnection->onSymbWriteEvent.connect(sigc::mem_fun(*this, &DmaEthMonitor::onSymbWrite));
    onPeripheralModelLearningConnection->onSymbReadEvent.connect(sigc::mem_fun(*this, &DmaEthMonitor::onSymbRead));
    s2e()->getCorePlugin()->onTranslateInstructionStart.connect(sigc::mem_fun(*this, &DmaEthMonitor::onTranslateInst));
    getInfoStream() << " --- Initialize the register for DMA ETH --- " << "\n";
    // set RCC HSERDY & PLLRDY
    //set_reg_value(g_s2e_state, 0x40023800, 0x2020000);
    // set RCC CFGR
    // when set the SW in RCC_CFGR by software the hardware should set the SWS but we can set it in initialization.
    //set_reg_value(g_s2e_state, 0x40023808, 0x8); 
    // set PWR ODRDY & ODSWRDY
    //set_reg_value(g_s2e_state, 0x40007004, 0x30000);

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
    DECLARE_PLUGINSTATE(DmaEthMonitorState, state);

    // bool ok = true;

    //test 
    getWarningsStream(state) << "write to address " << hexval(address) << " value " << hexval(concreteValue) << "\n";
    // PLL on
    if( address == 0x42470060 && concreteValue & 0x1 ) {
        getWarningsStream(state) << " set pll on physaddress " << hexval(address) << " value " << hexval(concreteValue) << "\n" ;
        plgState->set_pll_on();
    } else if (address == 0x420E0040 && concreteValue & 0x1 ) {
        getWarningsStream(state) << " set odr on physaddress " << hexval(address) << " value " << hexval(concreteValue) << "\n" ;
        plgState->set_odr_on();
    } else if (address == 0x420E0044 && concreteValue & 0x1 ) {
        getWarningsStream(state) << " set odswrdy on physaddress " << hexval(address) << " value " << hexval(concreteValue) << "\n" ;
        plgState->set_ODSWRDY_on();
    } else if (address == 0x40023808)// && concreteValue <  4 ) { // set SW in CFGR
        getWarningsStream(state) << " get SW in CFGR: value " << hexval(concreteValue) << "\n";
        plgState->set_CFGR_SW_val(concreteValue);
    }
    // Change invoked by conditions 
    // if( address == 0x40023804) { //ETH_MACFFR
    //     ok &= set_reg_value(state, 0x40023800, 0x1, 25, 1);
    // }else if( address == 0x40029000 && concreteValue&0x1) { // set ETH_MAC_MABMR
    //     ok &= set_reg_value(state, address, 0);
    // } else if (address == 0x40028010 && concreteValue&0x1) { // MACMIIAR
    //     ok &= set_reg_value(state, address, concreteValue&~0x1);
    // } else if (address == 0x400380C) { // receive descriptor list address
    //     getInfoStream(g_s2e_state) << "Get RxDesc Address " << concreteValue <<  "\n";
    //     Prepare_Rx_Desc(state, concreteValue);
    //     read_from_RxDesc(state, concreteValue);
    // }                

}

void DmaEthMonitor::onSymbRead(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                     uint64_t concreteValue, void *opaque, uint32_t *return_value) {
    
    DECLARE_PLUGINSTATE(DmaEthMonitorState, state);

    if ( address == 0x40023800)  { 
        if ( concreteValue & 0x10000 ) { // when HSE on read HSE need to be HSE rdy
            *return_value = concreteValue | 0x20000;
            getWarningsStream(state) << "HSE RDY when HSE on " << *return_value << "\n" ;
        } 
        if ( plgState->get_pll() ) {
            *return_value = concreteValue | 0x2000000;
            getWarningsStream(state) << "PLL RDY when PLL on" << *return_value << "\n";
        }
    } else if ( address == 0x40007004 && (plgState->get_odr() || plgState->get_ODSWRDY()) ) {
        *return_value = concreteValue | 0x30000;
        getWarningsStream(state) << "ODRDY&ODSWRDY when on" << *return_value << "\n";
    } else if ( address == 0x40023808) { // get SW in CFGR
        *return_value = concreteValue |  ( plgState->get_CFGR_SW_val() << 2 );
        getWarningsStream(state) << " SWS is same to SW in CFGR: value " << *return_value << "\n";
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

void DmaEthMonitor::onTranslateInst( ExecutionSignal *signal, S2EExecutionState *state, TranslationBlock *tb, uint64_t pc) {
    if ( pc == 0x8000CA0 ) { // lwip_init()
        s2e()->getWarningsStream() << "check the address of exit : " << hexval(pc) << "\n"; 
        signal->connect(sigc::mem_fun(*this, &DmaEthMonitor::onInstExec));
    } 
}

void DmaEthMonitor::onInstExec(S2EExecutionState *state, uint64_t pc ) {
    s2e()->getWarningsStream() << "time to  exit : " << hexval(pc) << "\n"; 
    exit(0);
}


} // namespace hw
} // namespace plugins
} // namespace s2e
