param(
    [Parameter(Mandatory = $true)]
    [string]$File,

    [string]$ClangFormat = 'clang-format.exe'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $File -PathType Leaf)) {
    throw "No existe el archivo: $File"
}

# Resolve clang-format.exe either from an explicit path or from PATH.
if ([System.IO.Path]::IsPathRooted($ClangFormat)) {
    if (-not (Test-Path -LiteralPath $ClangFormat -PathType Leaf)) {
        throw "No existe clang-format.exe: $ClangFormat"
    }
    $clangFormatExe = (Resolve-Path -LiteralPath $ClangFormat).Path
}
else {
    $command = Get-Command $ClangFormat -ErrorAction Stop
    $clangFormatExe = $command.Source
}

Write-Host "ClangFormat: $clangFormatExe"
Write-Host "Archivo:     $File"

# First run the normal ClangFormat pass using the nearest .clang-format.
& $clangFormatExe '-style=file' '-i' '--' $File
if ($LASTEXITCODE -ne 0) {
    throw "clang-format.exe terminó con código $LASTEXITCODE"
}

# Read the file while preserving UTF-8 BOM and existing LF/CRLF style.
$bytes = [System.IO.File]::ReadAllBytes($File)
$hasUtf8Bom = $bytes.Length -ge 3 -and
    $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF

$encoding = New-Object System.Text.UTF8Encoding($hasUtf8Bom)
$text = $encoding.GetString($bytes)

$eol = if ($text.Contains("`r`n")) {
    "`r`n"
} elseif ($text.Contains("`n")) {
    "`n"
} else {
    "`r`n"
}

$lines = [System.Text.RegularExpressions.Regex]::Split($text, "`r`n|`n|`r")
$output = New-Object System.Collections.Generic.List[string]

for ($i = 0; $i -lt $lines.Count; $i++) {
    $line = $lines[$i]
    $trimmed = $line.Trim()

    $output.Add($line)

    # 1) Blank line after a block-opening brace.
    #    We only target braces at the end of a line, which is how clang-format
    #    emits normal C++ blocks with BreakBeforeBraces: Attach.
    $isOpeningBlock =
        $trimmed.EndsWith('{') -and
        -not $trimmed.StartsWith('//') -and
        -not $trimmed.StartsWith('/*')

    if ($isOpeningBlock) {
        if ($i + 1 -lt $lines.Count -and $lines[$i + 1].Trim() -ne '') {
            $output.Add('')
        }
        continue
    }

    # 2) Blank line after a closing block when the next statement starts a
    #    new logical piece of code. Do not separate } else / } catch / } while
    #    and do not add a blank before another closing brace.
    $isBareClosingBrace = $trimmed -match '^}$'
    if ($isBareClosingBrace) {
        if ($i + 1 -lt $lines.Count) {
            $nextTrimmed = $lines[$i + 1].Trim()
            if (
                $nextTrimmed -ne '' -and
                $nextTrimmed -notmatch '^(else\b|catch\b|while\b|})' -and
                $nextTrimmed -notmatch '^//'
            ) {
                $output.Add('')
            }
        }
        continue
    }

    # 3) In the requested style, separate a braceless continue/break from
    #    the next statement. Extend this regex if you want the same behavior
    #    for return/throw later.
    $isStandaloneFlowControl = $trimmed -match '^(continue|break);$'
    if ($isStandaloneFlowControl) {
        if ($i + 1 -lt $lines.Count -and $lines[$i + 1].Trim() -ne '' -and $lines[$i + 1].Trim() -ne '}') {
            $output.Add('')
        }
    }
}

# Avoid adding an extra newline beyond what the source already had.
$result = [string]::Join($eol, $output)

if ($text.EndsWith("`r`n")) {
    if (-not $result.EndsWith("`r`n")) { $result += "`r`n" }
}
elseif ($text.EndsWith("`n")) {
    if (-not $result.EndsWith("`n")) { $result += "`n" }
}

[System.IO.File]::WriteAllText($File, $result, $encoding)
Write-Host 'Formato aplicado correctamente.'
