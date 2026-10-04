[CmdletBinding()]
param(
    [ValidateRange(0, 65535)]
    [int] $ExternalPixelCount = 0,

    [ValidatePattern('^COM\d+$')]
    [string] $Port,

    [switch] $VerboseOutput,

    [switch] $UploadOnly,

    [switch] $Clean
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

if ($UploadOnly -and -not $Port) {
    throw "-UploadOnly requires -Port."
}
if ($UploadOnly -and $Clean) {
    throw "-UploadOnly cannot be combined with -Clean."
}

$coreList = arduino-cli core list
if ($LASTEXITCODE -ne 0) {
    throw "Unable to list installed Arduino cores."
}

$esp32Core = $coreList | Select-String -Pattern '^esp32:esp32\s+(\S+)'
if (-not $esp32Core) {
    throw "The esp32:esp32 Arduino core is not installed."
}

$coreVersion = $esp32Core.Matches[0].Groups[1].Value
$arduinoData = Join-Path $env:LOCALAPPDATA "Arduino15"
$env:ARDUINO_ESP32_PLATFORM_PATH =
    Join-Path $arduinoData "packages\esp32\hardware\esp32\$coreVersion"

if (-not (Test-Path $env:ARDUINO_ESP32_PLATFORM_PATH)) {
    throw "ESP32 core files were not found at $env:ARDUINO_ESP32_PLATFORM_PATH."
}

python -m esptool version *> $null
if ($LASTEXITCODE -ne 0) {
    throw "Python package 'esptool' is required. Install it with: python -m pip install esptool"
}

$toolPath = Join-Path $repoRoot "tools"
$outputDirectory = if ($ExternalPixelCount -gt 0) {
    Join-Path $repoRoot "build\onboard-plus-p2-$ExternalPixelCount-pixels"
} else {
    Join-Path $repoRoot "build\onboard-only"
}
$buildDirectory = Join-Path $repoRoot "build\arduino-work"
$successMarker = Join-Path $buildDirectory ".build-succeeded"

$arguments = @(
    "compile"
    "--fqbn"
    "esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,USBMode=hwcdc,CDCOnBoot=cdc"
    "--libraries"
    (Join-Path $repoRoot "libraries")
    "--output-dir"
    $outputDirectory
    "--build-path"
    $buildDirectory
    "--build-property"
    "tools.esptool_py.path=$toolPath"
    "--build-property"
    "tools.esptool_py.cmd.windows=esptool.cmd"
    "--build-property"
    "tools.gen_esp32part.cmd.windows=$toolPath\gen_esp32part.cmd"
    "--build-property"
    "tools.gen_insights_pkg.cmd.windows=$toolPath\gen_insights_package.cmd"
)

$extraFlags = @()
if ($ExternalPixelCount -gt 0) {
    $extraFlags += "-DCOLOR_CONTROLLER_EXTERNAL_PIXEL_COUNT=$ExternalPixelCount"
}
if ($extraFlags.Count -gt 0) {
    $arguments += @(
        "--build-property"
        "compiler.cpp.extra_flags=$($extraFlags -join ' ')"
    )
}

if ($VerboseOutput) {
    $arguments += "--verbose"
}

$arguments += Join-Path $repoRoot "src\ColorController"

$mutex = [System.Threading.Mutex]::new($false, "Local\color-controller-firmware-build")
$lockAcquired = $false
$exitCode = 1
try {
    try {
        $lockAcquired = $mutex.WaitOne([TimeSpan]::FromMinutes(10))
    } catch [System.Threading.AbandonedMutexException] {
        $lockAcquired = $true
    }
    if (-not $lockAcquired) {
        throw "Timed out waiting for another color-controller firmware build to finish."
    }

    if (-not $UploadOnly) {
        if ($Clean -or -not (Test-Path -LiteralPath $successMarker -PathType Leaf)) {
            $arguments += "--clean"
        }
        Remove-Item -LiteralPath $successMarker -Force -ErrorAction SilentlyContinue
        & arduino-cli @arguments
        $exitCode = $LASTEXITCODE
        if ($exitCode -eq 0) {
            New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
            Set-Content -LiteralPath $successMarker -Value (Get-Date -Format o)
        }
    } else {
        $exitCode = 0
    }

    if ($exitCode -eq 0 -and $Port) {
        $bootloader = Join-Path $outputDirectory "ColorController.ino.bootloader.bin"
        $partitions = Join-Path $outputDirectory "ColorController.ino.partitions.bin"
        $application = Join-Path $outputDirectory "ColorController.ino.bin"
        $bootApp = Join-Path $env:ARDUINO_ESP32_PLATFORM_PATH "tools\partitions\boot_app0.bin"
        $requiredImages = @($bootloader, $partitions, $application, $bootApp)
        $missingImages = $requiredImages | Where-Object {
            -not (Test-Path -LiteralPath $_ -PathType Leaf)
        }
        if ($missingImages) {
            throw "Required firmware image not found: $($missingImages -join ', '). Build first without -UploadOnly."
        }
        & python -m esptool `
            --chip esp32s3 `
            --port $Port `
            --baud 921600 `
            --before default-reset `
            --after hard-reset `
            write-flash `
            --flash-mode keep `
            --flash-freq keep `
            --flash-size keep `
            0x0 `
            $bootloader `
            0x8000 `
            $partitions `
            0xe000 `
            $bootApp `
            0x10000 `
            $application
        $exitCode = $LASTEXITCODE
    }
} finally {
    if ($lockAcquired) {
        $mutex.ReleaseMutex()
    }
    $mutex.Dispose()
}

exit $exitCode
