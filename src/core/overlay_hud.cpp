#include "core/module_interfaces.hpp"

// ============================================================================
// STUB DE PARTE B — Ver TASKS.md [TASK-B4] (rama sugerida: feat/part-b-overlay-benchmark)
// ============================================================================

namespace WinOpty::Modules {

Core::Result InGameOverlay::Start() {
    // TODO(TASK-B4): Usar ventana overlay transparente topmost + ETW/PresentMon
    // (evitar inyeccion DLL en juegos con anti-cheat).
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "InGameOverlay::Start pendiente en Parte B [TASK-B4]");
}

Core::Result InGameOverlay::Stop() {
    return Core::Result::Success();
}

bool InGameOverlay::IsRunning() const {
    return false;
}

void BenchmarkSession::Begin() {
    fpsMin = 0.0f;
    fpsMax = 0.0f;
    fpsAvg = 0.0f;
    fps1PercentLow = 0.0f;
    fps0_1PercentLow = 0.0f;
    totalFrames = 0;
    timeAbove60Pct = 0.0f;
    timeAbove120Pct = 0.0f;
}

void BenchmarkSession::RecordFrameMs(double frameTimeMs) {
    (void)frameTimeMs;
    // TODO(TASK-B4): Almacenar frametimes para calcular percentiles 1% Low y 0.1% Low.
}

void BenchmarkSession::Finalize() {
    // TODO(TASK-B4): Calcular metricas finales de sesion.
}

Core::Result BenchmarkSession::ExportCsv(const std::string& filePath) const {
    (void)filePath;
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "BenchmarkSession::ExportCsv pendiente en Parte B [TASK-B4]");
}

Core::Result BenchmarkSession::ExportJson(const std::string& filePath) const {
    (void)filePath;
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "BenchmarkSession::ExportJson pendiente en Parte B [TASK-B4]");
}

Core::Result ScreenshotCapture::CaptureActiveWindow(const std::string& outputDir) {
    (void)outputDir;
    // TODO(TASK-B4): Capturar framebuffer/ventana activa por hotkey (F12) en PNG/WebP.
    return Core::Result::Fail(Core::StatusCode::NotImplemented,
        "ScreenshotCapture::CaptureActiveWindow pendiente en Parte B [TASK-B4]");
}

} // namespace WinOpty::Modules
