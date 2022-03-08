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

static bool symbhw_symbmonitor(struct MemoryDesc *mr, uint64_t physaddress, uint64_t value, unsigned size,
                               unsigned flag, void *opaque);

S2E_DEFINE_PLUGIN(DmaMonitor, "Example s2e plugin", "DmaMonitor", "PeripheralModelLearning");

namespace {
class DmaMonitorState : public PluginState {
private:
    int m_flag;
    
    // MemoryReadArrays memory_read_arrays;
    MemRangeMap mem;
    // MemoryAccessArrays m_mem_access_arrays;

public:
    DmaMonitorState() {
        m_flag = 0;
    }

    virtual ~DmaMonitorState() {
    }

    static PluginState *factory(Plugin *, S2EExecutionState *) {
        return new DmaMonitorState();
    }

    DmaMonitorState *clone() const {
        return new DmaMonitorState(*this);
    }

    // capture the address configure 
    bool is_Dma(uint64_t value, MemRangeMap mem) {
        // for (MemRangeMap::iterator it = mem.begin(); it != mem.end(); it++) {
        //     if (value >= it->second.first && value <= it->second.second) {
        //         return true;
        //     }
        // }
        bool addressDMAflag = false;
        if(value >= mem[TRAM].first && value <= mem[TRAM].second){
            addressDMAflag = true;
        }

        return addressDMAflag;
    }

    PointerType get_pointer_type(uint64_t value, MemRangeMap mem) {
        if (value >= mem[TFLASH].first && value <= mem[TFLASH].second) {
            return TFLASH;
        } else if (value >= mem[TRAM].first && value <= mem[TRAM].second) {
            return TRAM;
        } else if (value >= mem[TPERIPHERAL].first && value <= mem[TPERIPHERAL].second) {
            return TPERIPHERAL;
        }
        return TUNKNOWN;
    }

    bool valid_descriptor_pointers(uint64_t p1, uint64_t p2, MemRangeMap mem) {
        PointerType pt1, pt2;

        pt1 = get_pointer_type(p1, mem);
        pt2 = get_pointer_type(p2, mem);

        if (pt1 == TUNKNOWN || pt2 == TUNKNOWN)
            return false;

        if (pt1 == TFLASH && pt2 == TFLASH)
            return false;
        if (pt1 == TFLASH && pt2 == TRAM)
            return false; // this should be true but generates too many false positives
        if (pt1 == TFLASH && pt2 == TPERIPHERAL)
            return false;

        if (pt1 == TRAM && pt2 == TFLASH)
            return false; // this should be true but generates too many false positives
        if (pt1 == TRAM && pt2 == TRAM)
            return true; // this should be true but generates too many false positives
        if (pt1 == TRAM && pt2 == TPERIPHERAL)
            return true;

        if (pt1 == TPERIPHERAL && pt2 == TFLASH)
            return false;
        if (pt1 == TPERIPHERAL && pt2 == TRAM)
            return true;
        if (pt1 == TPERIPHERAL && pt2 == TPERIPHERAL)
            return false; // this should be true but we are not able to emulate this type of DMA transfer
        return false;
    }
/*
    void init_mem_access_arrays(uint64_t physaddress, unsigned size, uint64_t value, unsigned flag, PointerType type){
        MemoryAccessArray new_array;
        new_array.push_back(physaddress);
        pm_dma_access_type_t t_access_mem;
        if(flag)
            t_access_mem = READ;
        else
            t_access_mem = WRITE;
        // PointerType type = get_pointer_type(physaddress);
        MemAccessDesc new_desc = {new_array, size, 1, type, t_access_mem, new_array.begin(), new_array.end()};
        m_mem_access.push_back(new_desc);
    }

    bool update_mem_access(uint64_t physaddress){
        bool flag = false;
        for(MemAccessMap::iterator it=mem_access.begin(); it!=mem_access.end(); it++){
            if(physaddress == ((*it).end_address + (*it).beat_size))
                flag = true;
        }
        return flag;
    }

    void update_mem_access_arrays(uint64_t physaddress, unsigned size, uint64_t value, unsigned flag, PointerType type){

    }
*/

};
} // namespace

void DmaMonitor::initialize() {
    // bool ok;
    if (!parseConfigIoT()) {
        getWarningsStream() << "Could not parse peripheral config\n";
        exit(-1);
    }

    // This indicates that our plugin is interested in monitoring instruction translation.
    // For this, the plugin registers a callback with the onPeripheralModelLearning signal.

    onPeripheralModelLearningConnection = s2e()->getPlugin<PeripheralModelLearning>();
    onPeripheralModelLearningConnection->onSymbWriteEvent.connect(sigc::mem_fun(*this, &DmaMonitor::onSymbWrite));
    onPeripheralModelLearningConnection->onSymbReadEvent.connect(sigc::mem_fun(*this, &DmaMonitor::onSymbRead));

    // onInvalidStateDectionConnection->onInvalidStatesEvent.connect(
    //     sigc::mem_fun(*this, &PeripheralModelLearning::onInvalidStatesDetection));
    s2e()->getCorePlugin()->onTranslateInstructionStart.connect(
        sigc::mem_fun(*this, &DmaMonitor::onTranslateInstruction));

    g_symbolicMemoryMonitorHook = SymbolicMemoryMonitorHook(symbhw_is_mem_monitor, symbhw_symbmonitor, this);
}

bool DmaMonitor::parseConfigIoT(void) {
    ConfigFile *cfg = s2e()->getConfig();
    auto keys = cfg->getListKeys(getConfigKey());

    // ARM flash range 0x08000000-0x0807FFFF
    m_mem_range[TFLASH].first = 0x08000000;
    m_mem_range[TFLASH].second = 0x0807FFFF;
    getDebugStream() << "Adding monitor flash range: " << hexval(m_mem_range[TFLASH].first) << " - " << hexval(m_mem_range[TFLASH].second)
                     << "\n";

    // ARM SRAM range 0x20000000-0x2000FFFF
    m_mem_range[TRAM].first = 0x20000000;
    m_mem_range[TRAM].second = 0x2000FFFF;
    getDebugStream() << "Adding monitor sram range: " << hexval(m_mem_range[TRAM].first) << " - " << hexval(m_mem_range[TRAM].second) << "\n";

    // ARM peripheral range 0x40000000-0x5FFFFFFF
    m_mem_range[TPERIPHERAL].first = 0x40000000;
    m_mem_range[TPERIPHERAL].second = 0x5FFFFFFF;
    getDebugStream() << "Adding monitor peripheral range: " << hexval(m_mem_range[TPERIPHERAL].first) << " - "
                     << hexval(m_mem_range[TPERIPHERAL].second) << "\n";

    return true;
}

static bool symbhw_is_mem_monitor(struct MemoryDesc *mr, uint64_t physaddr, uint64_t size, void *opaque) {
    DmaMonitor *hw = static_cast<DmaMonitor *>(opaque);
    return hw->isMemMonitor(physaddr);
}

static bool symbhw_symbmonitor(struct MemoryDesc *mr, uint64_t physaddress, uint64_t value, unsigned size,
                               unsigned flag, void *opaque) {
    DmaMonitor *hw = static_cast<DmaMonitor *>(opaque);

    hw->getDebugStream(g_s2e_state) << "------Go into SizeLearning Mode------ " << "\n";


    hw->onSizeLearningMode(g_s2e_state, physaddress, size, value, flag);
    // unsigned size = value->getWidth() / 8;
    // uint64_t concreteValue = g_s2e_state->toConstantSilent(value)->getZExtValue();

    return true;
    // return hw->onDetectingMode(g_s2e_state, SYMB_MMIO, physaddress, size, concreteValue);
}

void DmaMonitor::onSizeLearningMode(S2EExecutionState *state, uint64_t address, unsigned size, uint64_t concreteValue, unsigned flag){
    // DECLARE_PLUGINSTATE(DmaMonitorState, state);
    // PointerType type = plgState->get_pointer_type(address, m_mem_range);
    uint64_t beatsize = (uint64_t)size;
    bool cflag = true;

    s2e()->getDebugStream(g_s2e_state) << "------address------ " << hexval(address) << "------size------ "
                                    << hexval(beatsize) << "------value------ " << hexval(concreteValue) << "\n";
    if(m_mem_access.empty()){
        // plgState->init_mem_access_array(address, size, concreteValue, flag, type);
        m_mem_access[address] = address;
        m_mem_access_beatsize[address] = beatsize;
        m_mem_access_check[address] = true;
    }
    
    for(MemAccessMap::iterator it=m_mem_access.begin(); it!=m_mem_access.end();it++){
        
        if(address == (it->second + m_mem_access_beatsize[it->first]) && (m_mem_access_check.find(address) == m_mem_access_check.end())){
            it->second = address;
            cflag = false;
            // s2e()->getWarningsStream(g_s2e_state) << hexval(it->second + m_mem_access_beatsize[it->first]) << "\n";
        }

        if(!m_CMARx_found.empty()){
            for(DmaAddress::iterator itx=m_CMARx_found.begin(); itx!=m_CMARx_found.end(); itx++){
                if(*itx == it->first)
                    s2e()->getWarningsStream(g_s2e_state) << "------address pair------ " << hexval(it->first) << "----------> "
                                    << hexval(it->second)  << "\n";
            }
        }
    }

    if(cflag){
        m_mem_access[address] = address;
        m_mem_access_beatsize[address] = beatsize;
        m_mem_access_check[address] = true;
    }

}

bool DmaMonitor::isMemMonitor(uint64_t physaddress) {
    uint64_t m_address = physaddress;
    return isMonitor(m_mem_range, m_address);
}

// a more aggressive filter
// template <typename T, typename U> inline bool DmaMonitor::isMonitor(T mem, U address) {
//     // for (auto &p : ports) {
//     //     if (port >= p.first && port <= p.second) {
//     //         return true;
//     //     }
//     // }
//     bool monitorflag = false;
//     bool flag = false; // check whether the address has been accessed or not

//     for(MemoryMonitorArray::iterator it=m_mem_array.begin(); it!=m_mem_array.end();it++){
//         if(address == (*it)){
//             flag = true;
//             return monitorflag;
//         }
//     }

//     if(!flag && (address >= mem[TRAM].first && address<= mem[TRAM].second) ){
//         m_mem_array.push_back(address);
//         monitorflag = true;
//     }

//     return monitorflag;
// } 

// activate when address in ram
template <typename T, typename U> inline bool DmaMonitor::isMonitor(T mem, U address) {
    if (address >= mem[TRAM].first && address<= mem[TRAM].second) {
        return true;
    }
    return false;
}

void DmaMonitor::onSymbWrite(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                             uint64_t concreteValue, void *opaque) {
    DECLARE_PLUGINSTATE(DmaMonitorState, state);
    // This function get Symbolic write info and sent the info to onDetectingMode
    DmaMonitor *hw = static_cast<DmaMonitor *>(opaque);
    uint64_t physaddress = address;
    bool flag = true;

    if (plgState->is_Dma(concreteValue, m_mem_range)) {
        hw->getWarningsStream(g_s2e_state) << "------DMA configure found-------" << '\n';
        s2e()->getWarningsStream() << "------DMA_CMARx address------- " << hexval(address) << '\n';
        s2e()->getWarningsStream() << "------DMA_CMARx value------- " << hexval(concreteValue) << '\n';


        for(DmaAddress::iterator it=m_CMARx_found.begin(); it!=m_CMARx_found.end(); it++){
            if((*it)==concreteValue)
                flag = false;
        }

        if(flag)
            m_CMARx_found.push_back(concreteValue);
            
        hw->onDetectingMode(g_s2e_state, SYMB_DMA, physaddress, size, concreteValue);
    }
}

void DmaMonitor::onSymbRead(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                            uint64_t concreteValue, void *opaque) {

    if (concreteValue == 0x40020050) {
        s2e()->getWarningsStream() << "------1------- " << hexval(address) << '\n';
        s2e()->getWarningsStream() << "------2------- " << hexval(concreteValue) << '\n';
    }
}

// klee::ref<klee::Expr> DmaMonitor::onDetectingMode(S2EExecutionState *state, SymbolicHardwareAccessType type,
//                                                   uint64_t address, unsigned size, uint64_t concreteValue) {
void DmaMonitor::onDetectingMode(S2EExecutionState *state, SymbolicHardwareAccessType type,
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

    
    getWarningsStream(g_s2e_state) << ss.str() << " size " << hexval(size) << " value=" << hexval(concreteValue)
                                   << " sym=" << (createVariable ? "yes" : "no") << "\n";

    // if (createVariable) {
    //     ConcreteArray concolicValue;
    //     // SymbHwGetConcolicVector(concreteValue, size, concolicValue);
    //     return klee::ConstantExpr::create(0x0, size * 8);
    //     // return state->createSymbolicValue(ss.str(), size * 8, concolicValue);
    // } else {
    //     return klee::ExtractExpr::create(klee::ConstantExpr::create(concreteValue, 64), 0, size * 8);
    // }
}

void DmaMonitor::onTranslateInstruction(ExecutionSignal *signal, S2EExecutionState *state, TranslationBlock *tb,
                                        uint64_t pc) {
    // DECLARE_PLUGINSTATE(DmaMonitorState, state);

    s2e()->getDebugStream() << "Executing instruction at " << hexval(pc) << '\n';
}

} // namespace hw
} // namespace plugins
} // namespace s2e
