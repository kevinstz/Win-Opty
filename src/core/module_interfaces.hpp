#pragma once

#include "core/engine_config.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace WinOpty::Modules {

// ============================================================================
// Contratos compartidos entre Parte A y Parte B
// Ver docs/ARCHITECTURE.md y TASKS.md antes de cambiar firmas publicas.
// ============================================================================

// 1. Limpieza Automatica y Manual de RAM (Implementado en Parte A)
struct MemoryCleaner {
    bool clearSystemWorkingSet  = true;
    bool clearStandbyPageLists  = true;
    bool clearModifiedPageLists = false;
    uint32_t autoCleanThreshold = 75;
    uint32_t intervalSeconds    = 300;

    Core::Result ExecuteTrim();
    double GetCurrentMemoryLoadPercent() const;
    bool ShouldAutoClean() const;
};

// 2. Overlay de FPS en Juego (Implementado en Parte B - TASK-B4)
struct InGameOverlay {
    bool showFPS            = true;
    bool showFrametimeGraph = true;
    bool showCpuGpuMetrics  = true;
    bool showMemoryMetrics  = true;
    int32_t positionX       = 24;
    int32_t positionY       = 24;

    Core::Result Start();
    Core::Result Stop();
    bool IsRunning() const;
};

// 3. Modo Benchmark Competitivo (Implementado en Parte B - TASK-B4)
struct BenchmarkSession {
    float fpsMin           = 0.0f;
    float fpsMax           = 0.0f;
    float fpsAvg           = 0.0f;
    float fps1PercentLow   = 0.0f;
    float fps0_1PercentLow = 0.0f;
    uint64_t totalFrames   = 0;
    float timeAbove60Pct   = 0.0f;
    float timeAbove120Pct  = 0.0f;

    void Begin();
    void RecordFrameMs(double frameTimeMs);
    void Finalize();
    Core::Result ExportCsv(const std::string& filePath) const;
    Core::Result ExportJson(const std::string& filePath) const;
};

struct CpuSnapshot {
    double totalLoadPct = 0.0;
    double packageTempC = 0.0;
    double effectiveClockMHz = 0.0;
    std::vector<double> perThreadLoadPct;
};

struct GpuSnapshot {
    std::string adapterName;
    double coreLoadPct = 0.0;
    double hotspotTempC = 0.0;
    double powerWatts = 0.0;
    uint64_t vramUsedBytes = 0;
    uint64_t vramTotalBytes = 0;
};

struct StorageNetSnapshot {
    double diskReadMBps = 0.0;
    double diskWriteMBps = 0.0;
    double netRxKBps = 0.0;
    double netTxKBps = 0.0;
};

// 4. Monitor de Recursos en Tiempo Real (Implementado en Parte B - TASK-B1)
struct HardwareMonitor {
    Core::Result PollCpuSensors();
    Core::Result PollGpuSensors();
    Core::Result PollStorageAndNet();

    const CpuSnapshot& LastCpu() const { return lastCpu_; }
    const GpuSnapshot& LastGpu() const { return lastGpu_; }
    const StorageNetSnapshot& LastIo() const { return lastIo_; }

private:
    CpuSnapshot lastCpu_;
    GpuSnapshot lastGpu_;
    StorageNetSnapshot lastIo_;
};

struct DetectedGame {
    std::string platform; // Steam, Epic, GOG, Ubisoft, EA, Battle.net, Rockstar
    std::string title;
    std::string executablePath;
    std::string launchArgs;
};

// 5. Lanzador de Juegos con Prioridad (Implementado en Parte B - TASK-B2)
struct PriorityLauncher {
    bool setHighCpuPriority = true;
    bool bindFastCoresOnly  = true;
    bool preCleanRamOnStart = true;

    std::vector<DetectedGame> ScanInstalledGames() const;
    Core::Result LaunchGame(const DetectedGame& game, uint32_t* outPid = nullptr);
};

// 6. Modo Juego Dinamico (Implementado en Parte B - TASK-B3)
struct DynamicGameMode {
    Core::Result OnGameStart(uint32_t pid);
    Core::Result OnGameExit(uint32_t pid);
};

struct CacheCleanSummary {
    uint64_t scannedBytes = 0;
    uint64_t deletedBytes = 0;
    uint64_t deletedFiles = 0;
    uint64_t skippedFiles = 0;
};

// 7. Limpieza de Cache y Disco (Implementado en Parte A)
struct CacheCleaner {
    bool cleanOsTempFiles   = true;
    bool cleanShaderCaches  = true;
    bool cleanBrowserCaches = false;
    bool cleanGameLogs      = true;

    // Permite inyectar una raiz personalizada para pruebas unitarias aisladas
    std::string testSandboxRoot;

    uint64_t ScanRecoverableBytes() const;
    Core::Result CleanSelected(bool dryRun, CacheCleanSummary* outSummary = nullptr) const;
};

// 8. Captura de Pantalla Instantanea (Implementado en Parte B - TASK-B4)
struct ScreenshotCapture {
    const char* hotkey = "F12";
    const char* format = "png";

    Core::Result CaptureActiveWindow(const std::string& outputDir = "./screenshots");
};

} // namespace WinOpty::Modules
