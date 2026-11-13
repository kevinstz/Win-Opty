#include "core/module_interfaces.hpp"

// ============================================================================
// STUB DE PARTE B — Ver TASKS.md [TASK-B1] (rama sugerida: feat/part-b-hardware-monitor)
// ============================================================================

namespace WinOpty::Modules {

Core::Result HardwareMonitor::PollCpuSensors() {
    // TODO(TASK-B1): Implementar lectura nativa de carga por hilo, reloj efectivo y temperatura.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "HardwareMonitor::PollCpuSensors pendiente en Parte B [TASK-B1]");
}

Core::Result HardwareMonitor::PollGpuSensors() {
    // TODO(TASK-B1): Implementar lectura de uso GPU, hotspot, potencia (W) y VRAM (DXGI/NVAPI/ADL).
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "HardwareMonitor::PollGpuSensors pendiente en Parte B [TASK-B1]");
}

Core::Result HardwareMonitor::PollStorageAndNet() {
    // TODO(TASK-B1): Implementar lectura de I/O NVMe/SSD y trafico TCP/UDP en vivo.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "HardwareMonitor::PollStorageAndNet pendiente en Parte B [TASK-B1]");
}

} // namespace WinOpty::Modules
