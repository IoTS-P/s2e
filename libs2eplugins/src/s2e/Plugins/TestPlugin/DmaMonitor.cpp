// From libs2ecore. Provides the hexval function
#include <boost/regex.hpp>
#include <klee/util/ExprUtil.h>
#include <s2e/ConfigFile.h>
#include <s2e/S2E.h>
#include <s2e/SymbolicHardwareHook.h>
#include <s2e/Utils.h>
#include <s2e/cpu.h>
#include <s2e/opcodes.h>

#include "DmaMonitor.h"

#include <llvm/Support/CommandLine.h>

using namespace klee;

// namespace {
// llvm::cl::opt<bool> DebugSymbHw("debug-symbolic-hardware", llvm::cl::init(true));
// }

namespace s2e {
namespace plugins {
namespace hw {

extern "C" {
static bool symbhw_is_mem_monitor(struct MemoryDesc *mr, uint64_t physaddr, uint64_t size, void *opaque);
}

static bool symbhw_symbmonitor(struct MemoryDesc *mr, uint64_t physaddress,
                                            uint64_t value,
                                            unsigned size,
                                            void *opaque);

S2E_DEFINE_PLUGIN(DmaMonitor, "Example s2e plugin", "DmaMonitor", "PeripheralModelLearning");

namespace {
class DmaMonitorState : public PluginState {
private:
    int m_flag;

public:
    DmaMonitorState() {
        m_flag = 0;
    }

    virtual ~DmaMonitorState() {

    }

    static PluginState *factory(Plugin*, S2EExecutionState*) {
        return new DmaMonitorState();
    }

    DmaMonitorState *clone() const {
        return new DmaMonitorState(*this);
    }

    // void increment() {
    //     ++m_count;
    // }

    // int get() {
    //     return m_count;
    // }
};
}

void DmaMonitor::initialize() {
    // bool ok;
    // if (!parseConfigIoT()) {
    //     getWarningsStream() << "Could not parse peripheral config\n";
    //     exit(-1);
    // }

    m_address = (uint64_t) s2e()->getConfig()->getInt(getConfigKey() + ".addressToTrack");

    // This indicates that our plugin is interested in monitoring instruction translation.
    // For this, the plugin registers a callback with the onPeripheralModelLearning signal.

    onPeripheralModelLearningConnection = s2e()->getPlugin<PeripheralModelLearning>();
    onPeripheralModelLearningConnection->onSymbWriteEvent.connect(
        sigc::mem_fun(*this, &DmaMonitor::onSymbWrite));
    onPeripheralModelLearningConnection->onSymbReadEvent.connect(
        sigc::mem_fun(*this, &DmaMonitor::onSymbRead));

    // onInvalidStateDectionConnection->onInvalidStatesEvent.connect(
    //     sigc::mem_fun(*this, &PeripheralModelLearning::onInvalidStatesDetection));

    g_symbolicMemoryMonitorHook = SymbolicMemoryMonitorHook(symbhw_is_mem_monitor, symbhw_symbmonitor, this);
}



static bool symbhw_is_mem_monitor(struct MemoryDesc *mr, uint64_t physaddr, uint64_t size, void *opaque) {
    DmaMonitor *hw = static_cast<DmaMonitor *>(opaque);
    hw->getWarningsStream(g_s2e_state) << "------isMemMonitor------- " << hexval(physaddr) << "--size--"<< size << '\n';
    return hw->isMemMonitor(physaddr);
}



static bool symbhw_symbmonitor(struct MemoryDesc *mr, uint64_t physaddress,
                                            uint64_t value,
                                            unsigned size,
                                            void *opaque){
    DmaMonitor *hw = static_cast<DmaMonitor *>(opaque);
    // physaddress = 0x80010b0;
    // if (DebugSymbHw) {
    hw->getWarningsStream(g_s2e_state) << "------address------ " << hexval(physaddress) <<
        "------size------ " << hexval(size) << "------value------ " << hexval(value) << "\n";
    // }

    // unsigned size = value->getWidth() / 8;
    // uint64_t concreteValue = g_s2e_state->toConstantSilent(value)->getZExtValue();

    return true;
    // return hw->onDetectingMode(g_s2e_state, SYMB_MMIO, physaddress, size, concreteValue);
}

bool DmaMonitor::isMemMonitor(uint64_t physaddress) {
    uint64_t m_address = 0x2000000C;
    // s2e()->getWarningsStream() << "------isMemMonitor------- " << hexval(physaddress) << '\n';
    return isMonitor(physaddress, m_address);
}

template <typename T> inline bool DmaMonitor::isMonitor(T physaddress, T m_address) {
    // for (auto &p : ports) {
    //     if (port >= p.first && port <= p.second) {
    //         return true;
    //     }
    // }
    // if(physaddress == m_address){
        
    if(physaddress == m_address){
        s2e()->getWarningsStream() << "------Monitor at------- " << hexval(physaddress) << '\n';
        return true;
    }

    return true;
}

void DmaMonitor::onSymbWrite(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address,
                                         unsigned size, uint64_t concreteValue, void *opaque) {
    
    // This function get Symbolic write info and sent the info to onDetectingMode
    DmaMonitor *hw = static_cast<DmaMonitor *>(opaque);
    uint64_t physaddress = address;
    // uint64_t tmpaddress = address;
    
    // ARM MMIO range 0x20000000-0x40000000
    SymbolicMmioRange m;

    m.first = 0x20000000;
    m.second = 0x3fffffff;

    // ARM Dma range 0x40020400-0x400203FF
    SymbolicMmioRange mDma1;

    mDma1.first = 0x40020000;
    mDma1.second = 0x400203FF;

    // s2e()->getWarningsStream() << "------1------- " << hexval(address) << '\n';


    // if (concreteValue >= m.first && concreteValue <= m.second){
    if (physaddress >= mDma1.first && physaddress <= mDma1.second){ 
        hw->getWarningsStream(g_s2e_state) << "------DMA configure found-------" << '\n';
        s2e()->getWarningsStream() << "------DMA_CPARx found------- " << hexval(address) << '\n';
        s2e()->getWarningsStream() << "------DMA_CPARx value------- " << hexval(concreteValue) << '\n';
        hw->onDetectingMode(g_s2e_state, SYMB_MMIO, physaddress, size, concreteValue);
    }

    if (concreteValue >= m.first && concreteValue <= m.second){ 
        hw->getWarningsStream(g_s2e_state) << "------DMA configure found-------" << '\n';
        s2e()->getWarningsStream() << "------DMA_CMARx found------- " << hexval(address) << '\n';
        s2e()->getWarningsStream() << "------DMA_CMARx value------- " << hexval(concreteValue) << '\n';
    }


    // The plugins can arbitrarily modify/observe the current execution state via the execution state pointer.
    // Plugins can also call the s2e() method to use the S2E API
    // This macro declares the plgState variable of type DmaMonitorState.
    // It automatically takes care of retrieving the right plugin state attached to the specified execution state
    // DECLARE_PLUGINSTATE(DmaMonitorState, state);

    // Increment the count
    // plgState->increment();

    // // Trigger the event
    // if ((plgState->get() % 10) == 0) {
    //     onPeriodicEvent.emit(state, pc);
    // }
}

void DmaMonitor::onSymbRead(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address,
                                         unsigned size, uint64_t concreteValue, void *opaque) {

    if (concreteValue == 0x40020050){
        s2e()->getWarningsStream() << "------1------- " << hexval(address) << '\n';
        s2e()->getWarningsStream() << "------2------- " << hexval(concreteValue) << '\n';


    }

}

klee::ref<klee::Expr> DmaMonitor::onDetectingMode(S2EExecutionState *state, SymbolicHardwareAccessType type,
                                                              uint64_t address, unsigned size, uint64_t concreteValue) {
    
    bool createVariable = true;
    // uint32_t phaddr = address;
    uint32_t pc = state->regs()->getPc();
    std::stringstream ss;
    switch (type) {
        case SYMB_MMIO:
            ss << "iommuread_";
            break;
        case SYMB_DMA:
            ss << "dmaread_";
            break;
        case SYMB_PORT:
            ss << "portread_";
            break;
    }

    ss << hexval(address) << "@" << hexval(pc);

    ss << " " << "@@@@@@@"<< " ";

    getWarningsStream(g_s2e_state) << ss.str() << " size " << hexval(size) << " value=" << hexval(concreteValue)
                                << " sym=" << (createVariable ? "yes" : "no") << "\n";

    if(concreteValue == 0x6ac1){
        ss << "dma start " << "@@@@@@@"<< " ";
    }

    if (createVariable) {
        ConcreteArray concolicValue;
        // SymbHwGetConcolicVector(concreteValue, size, concolicValue);
        return klee::ConstantExpr::create(0x0, size * 8);
        // return state->createSymbolicValue(ss.str(), size * 8, concolicValue);
    } else {
        return klee::ExtractExpr::create(klee::ConstantExpr::create(concreteValue, 64), 0, size * 8);
    }


}




} // namespace hw
} // namespace plugins 
} // namespace s2e
