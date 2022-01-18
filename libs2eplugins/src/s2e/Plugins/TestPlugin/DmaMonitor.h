#ifndef S2E_PLUGINS_DmaMonitor_H
#define S2E_PLUGINS_DmaMonitor_H

#include <deque>
#include <inttypes.h>
#include <s2e/CorePlugin.h>
#include <s2e/Plugin.h>
#include <s2e/Plugins/uEmu/PeripheralModelLearning.h>
#include <s2e/Plugins/uEmu/ARMFunctionMonitor.h>
#include <s2e/Plugins/uEmu/InvalidStatesDetection.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/SymbolicHardwareHook.h>
#include <vector>

#include <llvm/ADT/SmallVector.h>

namespace s2e {
namespace plugins {
enum DmaStatus { Enable, Disable };
enum pm_dma_transfer_dir_t { MEMTOPERI = 0, PERITOMEM, NODIR};// enum DMA transfer direction
enum pm_dma_access_type_t { READ = 0, WRITE, READWRITE, NOACCESS};// enum type of access to descriptor pointers (dereference)

namespace hw {
typedef std::vector<uint8_t> ConcreteArray;
typedef std::pair<uint64_t, uint64_t> SymbolicMmioRange;
typedef llvm::SmallVector<SymbolicMmioRange, 4> SymbolicMmioRanges;
// typedef std::vector<uint32_t> ConRegs;

typedef std::pair<uint32_t /* DmaCCRaddress */, uint32_t /* pc */> Dma;
typedef std::map<uint32_t /* DmaCCRaddress */, uint32_t /* kind of type */> TypeFlagDmaMap;
// typedef std::map

// typedef std::pair<std::pair<uint32_t /* peripheraladdress */, uint32_t /* memoryaddress */>, uint32_t /* size */> ConfigureDMA;
// typedef std::pair<uint32_t /* peripheraladdress */, uint32_t /* memoryaddress */> DmaAddress;


class DmaMonitor : public Plugin {
    S2E_PLUGIN
    uint64_t m_address;

private:
    SymbolicMmioRanges m_mmio;
    PeripheralModelLearning *onPeripheralModelLearningConnection;
    
    // template <typename T> bool parseRangeList(ConfigFile *cfg, const std::string &key, T &result);
    // bool parseConfigIoT();
    
public:
    DmaMonitor(S2E *s2e): Plugin(s2e) {
    }

    void initialize();
    bool isMemMonitor(uint64_t physAddr);
    void onSymbWrite(S2EExecutionState *state,
            SymbolicHardwareAccessType type,
            uint64_t address,
            unsigned size,
            uint64_t concreteValue,
            void *
            );

    void onSymbRead(S2EExecutionState *state,
            SymbolicHardwareAccessType type,
            uint64_t address,
            unsigned size,
            uint64_t concreteValue,
            void *
            );

    klee::ref<klee::Expr> onDetectingMode(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address,
                                         unsigned size, uint64_t concreteValue);
    template <typename T> inline bool isMonitor(T m_address, T physAddr);

};

} // namespace hw
} // namespace plugins
} // namespace s2e

#endif
