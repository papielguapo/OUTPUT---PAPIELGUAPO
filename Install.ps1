$ErrorActionPreference = 'Stop'
$source = Join-Path $PSScriptRoot 'Binaries\OUTPUT.vst3'
$vstRoot = Join-Path $env:CommonProgramFiles 'VST3'
$backupRoot = Join-Path $PSScriptRoot 'Backups'
$resultPath = Join-Path $PSScriptRoot 'Install-result.json'
$target = Join-Path $vstRoot 'OUTPUT.vst3'
$previous = @('PAPIELGUAPO - D2F SCOPE.vst3','OUTPUT.vst3')
$moved = @()
try {
    if (-not (Test-Path -LiteralPath (Join-Path $source 'Contents\x86_64-win\OUTPUT.vst3'))) { throw 'Missing validated OUTPUT bundle.' }
    $expectedId = 'ABCDEF019182FAEB5061706944325363'
    foreach ($name in $previous) {
        $existing = Join-Path $vstRoot $name
        if (Test-Path -LiteralPath $existing) {
            $resolved = [IO.Path]::GetFullPath((Resolve-Path -LiteralPath $existing).Path)
            if (-not $resolved.StartsWith($vstRoot + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected source path.' }
            if ((Get-Item -LiteralPath $existing).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Refusing a linked plugin folder.' }
            $metadata = Get-Content -LiteralPath (Join-Path $existing 'Contents\Resources\moduleinfo.json') -Raw
            if (-not $metadata.Contains($expectedId)) { throw 'Another plugin occupies the installation path.' }
        }
    }
    New-Item -ItemType Directory -Path $vstRoot,$backupRoot -Force | Out-Null
    foreach ($name in $previous) {
        $existing = Join-Path $vstRoot $name
        if (Test-Path -LiteralPath $existing) {
            $backup = [IO.Path]::GetFullPath((Join-Path $backupRoot ($name + '-' + (Get-Date -Format 'yyyyMMdd-HHmmssfff'))))
            if (-not $backup.StartsWith($backupRoot + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected backup path.' }
            Move-Item -LiteralPath $existing -Destination $backup
            $moved += @{original=$existing;backup=$backup}
        }
    }
    Copy-Item -LiteralPath $source -Destination $vstRoot -Recurse -Force
    $relative = 'Contents\x86_64-win\OUTPUT.vst3'
    $expected = (Get-FileHash -LiteralPath (Join-Path $source $relative) -Algorithm SHA256).Hash
    $actual = (Get-FileHash -LiteralPath (Join-Path $target $relative) -Algorithm SHA256).Hash
    if ($expected -ne $actual) { throw 'Installed binary does not match the validated build.' }
    @{status='installed';path=$target;backups=$moved;sha256=$actual;fileVersion=(Get-Item -LiteralPath (Join-Path $target $relative)).VersionInfo.FileVersion} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $resultPath -Encoding utf8
    exit 0
} catch {
    $reason = $_.Exception.Message
    if ($moved.Count -gt 0) {
        try {
            if (Test-Path -LiteralPath $target) {
                $partial = [IO.Path]::GetFullPath((Join-Path $backupRoot ('partial-OUTPUT-' + (Get-Date -Format 'yyyyMMdd-HHmmssfff'))))
                if (-not $partial.StartsWith($backupRoot + '\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected rollback path.' }
                Move-Item -LiteralPath $target -Destination $partial
            }
            foreach ($entry in $moved) { Move-Item -LiteralPath $entry.backup -Destination $entry.original }
        } catch { $reason += ' Rollback: ' + $_.Exception.Message }
    }
    @{status='failed';message=$reason;backups=$moved} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $resultPath -Encoding utf8
    exit 1
}
