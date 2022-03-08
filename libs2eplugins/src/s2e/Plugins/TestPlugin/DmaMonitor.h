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

#define MAX_POINTERS 64
#define MAX_DMA_DESC MAX_POINTERS / 2
// beat address should be at least 16kb stepper firmware uses 4kb
#define MAX_DMA_BEAT_ADDRESS 2048
//#define DMA_SPAN_BUFFER 64
#define DMA_SPAN_BUFFER 4
#define NONUM 255
#define NUMCANARIES 20

namespace s2e {
namespace plugins {

enum DmaStatus { Enable, Disable };
enum pm_dma_transfer_dir_t { MEMTOPERI = 0, PERITOMEM, NODIR }; // enum DMA transfer direction
enum pm_dma_access_type_t {READ = 0, WRITE, READWRITE, NOACCESS}; // enum type of access to descriptor pointers (dereference)
enum PointerType { TRAM = 0, TFLASH, TPERIPHERAL, TUNKNOWN }; // enum for DMA pointer type

namespace hw {

typedef struct {
    target_ulong address;
    pm_dma_access_type_t access;
    int exc_num_dma;
} pm_dma_buffer_beat;

typedef struct {
    target_ulong address;
    int size;
} pm_dma_canary;

typedef struct {
    int id;                     // unique id for each DMA pointer identified
    int id_ctp;                 // id of DMA pointer counterpart
    target_ulong base;          // base of peripheral
    target_ulong register_addr; // address of CR register that was written
    target_ulong value;         // value to be written on CR
    target_ulong ordinal;       // ordinal number at wich the write operation happened
    PointerType type;     // type of pointer according to major memory areas Peripheral, RAM, Flash

} pm_DMA_pointers;

typedef struct {
    int id_pointer_mem;  // first
    int id_pointer_peri; // second

    pm_DMA_pointers pointer_mem;
    pm_DMA_pointers pointer_peri;

    int beat_size;    // number of "beats" to be transfered  a beat could be 8, 16, 32 bit
    int number_beats; // beat_size x number_beats= transfer size in bytes
    pm_dma_access_type_t t_access_mem;
    pm_dma_access_type_t t_access_peri;
    pm_dma_transfer_dir_t direction;
    int exc_num_dma; // exception number IRQ
    int version;     // 1: single pointer (NRF52832) , 2: Double pointer (F103, F429)
} pm_DMA_desc;

typedef std::vector<uint64_t> DmaAddress;

typedef std::pair<uint64_t, uint64_t> MemRange;
typedef std::map<PointerType, MemRange> MemRangeMap;
typedef std::vector<uint64_t> MemoryMonitorArray; // the addresses related to DMA
typedef std::map<uint64_t, bool> MemoryAccessCheck;
// typedef std::vector<MemAccessDesc> MemoryAccessArrays;

typedef std::map<uint64_t /* start addr */, uint64_t /* end addr */> MemAccessMap;
typedef std::map<uint64_t /* start addr */, uint64_t /* beatsize */> MemAccessBeatsizeMap;

class DmaMonitor : public Plugin {
    S2E_PLUGIN

private:
    SymbolicMmioRanges m_mmio;
    MemRangeMap m_mem_range;    // memory range of Peripheral, RAM, Flash
    // MemoryAccessArrays m_mem_access; // record all mem access 
    MemAccessMap m_mem_access; // record all mem access 
    MemAccessBeatsizeMap m_mem_access_beatsize;  // record start address related access beatsize
    MemoryAccessCheck m_mem_access_check; // record whether the address has been accessed before
    MemoryMonitorArray m_mem_array;
    DmaAddress m_CMARx_found;
    PeripheralModelLearning *onPeripheralModelLearningConnection;

public:
    DmaMonitor(S2E *s2e) : Plugin(s2e) {
    }

    void initialize();
    bool parseConfigIoT(void);
    bool isMemMonitor(uint64_t physAddr);
    void onSymbWrite(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                     uint64_t concreteValue, void *);

    void onSymbRead(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address, unsigned size,
                    uint64_t concreteValue, void *);

    // klee::ref<klee::Expr> onDetectingMode(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address,
    //                                       unsigned size, uint64_t concreteValue);
    void onDetectingMode(S2EExecutionState *state, SymbolicHardwareAccessType type, uint64_t address,
                                          unsigned size, uint64_t concreteValue);
    void onSizeLearningMode(S2EExecutionState *state,uint64_t address,
                                          unsigned size, uint64_t concreteValue, unsigned flag);
    template <typename T, typename U> inline bool isMonitor(T mem, U address);
    void onTranslateInstruction(ExecutionSignal *signal, S2EExecutionState *state, TranslationBlock *tb, uint64_t pc);
};

} // namespace hw
} // namespace plugins
} // namespace s2e

#endif
