param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot)
)

$ErrorActionPreference = 'Stop'
$failures = [System.Collections.Generic.List[string]]::new()

function Require-Match([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) { $failures.Add($Message) }
}

function Require-Order([string]$Text, [string]$First, [string]$Second, [string]$Message) {
    $firstIndex = $Text.IndexOf($First, [System.StringComparison]::Ordinal)
    $secondIndex = $Text.IndexOf($Second, [System.StringComparison]::Ordinal)
    if (($firstIndex -lt 0) -or ($secondIndex -lt 0) -or ($firstIndex -ge $secondIndex)) { $failures.Add($Message) }
}

$core = [System.IO.File]::ReadAllText((Join-Path $RepoRoot 'src/rcore.c'))
$platform = [System.IO.File]::ReadAllText((Join-Path $RepoRoot 'src/platforms/rcore_dreamcast.c'))
$audio = [System.IO.File]::ReadAllText((Join-Path $RepoRoot 'src/raudio.c'))
$utils = [System.IO.File]::ReadAllText((Join-Path $RepoRoot 'src/utils.c'))
$cmake = [System.IO.File]::ReadAllText((Join-Path $RepoRoot 'src/CMakeLists.txt'))
$models = [System.IO.File]::ReadAllText((Join-Path $RepoRoot 'src/rmodels.c'))
$dcmesh = [System.IO.File]::ReadAllText((Join-Path $RepoRoot 'src/dc_mesh.c'))

Require-Match $core 'msg\[newDataSize\s*-\s*1\s*-\s*i\]' 'SHA1 must serialize all eight length bytes.'
Require-Match $core 'inputSize\s*%\s*4' 'Base64 decoder must reject non-quad input lengths.'
Require-Match $core 'AppendDirectoryPath' 'Directory enumeration must allocate paths on demand.'
Require-Match $core '\(gamepad\s*>=\s*0\)' 'Gamepad accessors must reject negative indices.'
Require-Match $core 'RAYLIB_LOCAL_BUILD_TAG' 'Core canary must use a stable compile-time build tag.'

Require-Match $platform 'Image\s+GetClipboardImage\s*\(void\)' 'Dreamcast must preserve the clipboard-image ABI symbol.'
Require-Match $platform 'void\s+SetGamepadVibration\s*\(' 'Dreamcast must preserve the vibration ABI symbol.'
Require-Match $platform 'glKosShutdown\s*\(' 'Dreamcast ClosePlatform must release GLdc state.'

Require-Order $audio 'ma_device_uninit(&AUDIO.System.device);' 'ma_mutex_uninit(&AUDIO.System.lock);' 'Audio device callback must stop before its mutex is destroyed.'
Require-Match $audio 'rewoundWithoutProgress' 'Streaming decoder refill loops must terminate after no-progress rewind.'
Require-Match $audio 'GetModFrameCountBounded' 'MOD duration discovery must have a termination bound.'
Require-Match $audio 'frameCount\s*>=\s*0' 'Audio buffer updates must reject negative frame counts.'
Require-Match $audio 'ma_mutex_lock\(&AUDIO\.System\.lock\);\s*double requestedRate = \(double\)buffer->converter\.sampleRateOut' 'Audio pitch must read converter state under the audio mutex.'

Require-Match $utils 'success\s*=\s*\(count\s*==\s*\(size_t\)dataSize\)\s*&&\s*\(result\s*==\s*0\)' 'SaveFileData must fail on partial writes.'
Require-Match $utils 'Text file partially loaded' 'Partial text reads must be diagnosed accurately.'
Require-Match $utils 'traceLog\(logType, \(text != NULL\)\? text : "", args\);\s*va_end\(args\);\s*return;' 'Custom TraceLog callbacks must retain ownership of fatal behavior.'

Require-Match $cmake 'raymath\.h\s+dc_mesh\.h\s+dcmesh\.h' 'CMake installs must retain the public DCMesh headers.'
Require-Match $models 'rmodelsIQMWeightedBonesValid\(fileBlendIndexes, fileBlendWeights' 'IQM load must validate weighted joint indices before model allocation.'
if (([regex]::Matches($dcmesh, 'dcMeshUnalignedColorWord\(src\)')).Count -ne 2) { $failures.Add('Both DCMesh color sync paths must use the alignment-safe native-word loader.') }
Require-Match $dcmesh 'ray_material\s*==\s*dc_material\s*\+\s*1' 'DCMesh validation must account for raylib default material slot 0.'

if ($failures.Count -ne 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Output 'audit002 core contract checks passed'
