<#
.SYNOPSIS
  Match a diff against the security knowledge base (KB-*.md) and list candidate entries for the reviewer.

.DESCRIPTION
  Deterministic pre-filter used by the review flow (Claude /code-review, Copilot Reviewer, ReviewerCodex).
  For every changed file it checks, per active KB entry:
    - language   : file extension vs entry `language` ("any" matches everything)
    - keywords   : case-sensitive substring of entry `code_keywords` in the diff's +/- lines
                   (whole file content for untracked files in uncommitted mode)
    - pattern    : entry `file_pattern` glob vs path - reported as a hint only, NOT required
                   (KB patterns use Armoury Crate naming and would miss GlideX files)
  An entry is a hit when language and at least one keyword match. The reviewer then reads the
  hit entries and applies their `## Detection` section; this script never decides a finding.

  Config: .github/security-kb.json (env GLIDEX_SECKB_PATH overrides localPath).
  Missing KB or parse errors only produce a warning; exit code is 0 so the review continues.

.EXAMPLE
  pwsh .github/scripts/Find-SecurityKbHits.ps1                       # latest commit
  pwsh .github/scripts/Find-SecurityKbHits.ps1 -Commit 3af6a554e
  pwsh .github/scripts/Find-SecurityKbHits.ps1 -Mode uncommitted
  pwsh .github/scripts/Find-SecurityKbHits.ps1 -Update               # clone/pull first (needs repo in config)
#>
[CmdletBinding()]
param(
    [ValidateSet('commit', 'uncommitted')]
    [string]$Mode = 'commit',
    [string]$Commit = 'HEAD',
    [switch]$Update,
    [switch]$Json
)

$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }
$Utf8 = New-Object System.Text.UTF8Encoding($false)

function Write-KbWarning([string]$Message) {
    Write-Output "Security KB: WARNING - $Message (review continues without KB check)"
}

# ---------- locate repo, config, KB ----------
$RepoRoot = (& git rev-parse --show-toplevel 2>$null)
if (-not $RepoRoot) { Write-KbWarning 'not inside a git repository'; exit 0 }
$RepoRoot = $RepoRoot.Trim()

$ConfigPath = Join-Path $RepoRoot '.github/security-kb.json'
if (-not (Test-Path -LiteralPath $ConfigPath)) { Write-KbWarning "config not found: $ConfigPath"; exit 0 }
$Config = [System.IO.File]::ReadAllText($ConfigPath, $Utf8) | ConvertFrom-Json

$KbPath = $Config.localPath
if ($env:GLIDEX_SECKB_PATH) { $KbPath = $env:GLIDEX_SECKB_PATH }
if (-not [System.IO.Path]::IsPathRooted($KbPath)) { $KbPath = Join-Path $RepoRoot $KbPath }
$KbPath = [System.IO.Path]::GetFullPath($KbPath)

$Corpora = @('entries'); if ($Config.corpora) { $Corpora = @($Config.corpora) }
$Statuses = @('active'); if ($Config.statuses) { $Statuses = @($Config.statuses) }
$Branch = 'main'; if ($Config.branch) { $Branch = $Config.branch }

# ---------- optional update (future: KB is a git repo) ----------
if ($Update) {
    if (-not $Config.repo) {
        Write-Output "Security KB: -Update skipped, 'repo' is not set in security-kb.json; using local snapshot."
    } elseif (-not (Test-Path -LiteralPath $KbPath)) {
        & git clone --depth 1 --branch $Branch $Config.repo $KbPath 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) { Write-KbWarning "git clone failed: $($Config.repo)"; exit 0 }
    } elseif (Test-Path -LiteralPath (Join-Path $KbPath '.git')) {
        & git -C $KbPath pull --ff-only 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) { Write-Output 'Security KB: WARNING - git pull failed (offline?); using existing snapshot.' }
    } else {
        Write-Output "Security KB: WARNING - $KbPath exists but is not a git clone (zip snapshot). Delete it so -Update can clone; using existing snapshot."
    }
}

if (-not (Test-Path -LiteralPath $KbPath)) { Write-KbWarning "knowledge base not found: $KbPath"; exit 0 }

$KbVersion = 'zip snapshot'
if (Test-Path -LiteralPath (Join-Path $KbPath '.git')) {
    $rev = (& git -C $KbPath log -1 --format='%h %cs' 2>$null)
    if ($rev) { $KbVersion = $rev.Trim() }
}

# ---------- parse KB entries ----------
function Get-Unquoted([string]$s) {
    $s = $s.Trim()
    if ($s.Length -ge 2 -and (($s[0] -eq '"' -and $s[-1] -eq '"') -or ($s[0] -eq "'" -and $s[-1] -eq "'"))) {
        return $s.Substring(1, $s.Length - 2)
    }
    return $s
}

function Remove-YamlComment([string]$s) {
    # strip ' # comment' outside quotes
    $inQ = $null
    for ($i = 0; $i -lt $s.Length; $i++) {
        $c = $s[$i]
        if ($inQ) { if ($c -eq $inQ) { $inQ = $null } }
        elseif ($c -eq '"' -or $c -eq "'") { $inQ = $c }
        elseif ($c -eq '#' -and ($i -eq 0 -or $s[$i - 1] -match '\s')) { return $s.Substring(0, $i).TrimEnd() }
    }
    return $s
}

function Read-KbEntry([string]$Path) {
    $lines = [System.IO.File]::ReadAllLines($Path, $Utf8)
    if ($lines.Count -eq 0 -or $lines[0].Trim() -ne '---') { return $null }
    $fm = @{}
    $listKey = $null
    for ($i = 1; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        if ($line.Trim() -eq '---') { break }
        if ($line -match '^([A-Za-z_][A-Za-z0-9_]*):\s*(.*)$') {
            $key = $Matches[1]
            $val = Remove-YamlComment $Matches[2]
            $listKey = $null
            if ($val -match '^\[(.*)\]$') {
                $fm[$key] = @($Matches[1] -split ',' | ForEach-Object { Get-Unquoted $_ } | Where-Object { $_ })
            } elseif ($val -eq '') {
                $fm[$key] = @()
                $listKey = $key
            } else {
                $fm[$key] = Get-Unquoted $val
            }
        } elseif ($listKey -and $line -match '^\s+-\s+(.*)$') {
            $item = Get-Unquoted (Remove-YamlComment $Matches[1])
            if ($item) { $fm[$listKey] = @($fm[$listKey]) + $item }
        }
    }
    if (-not $fm['id']) { return $null }
    $fm['_path'] = $Path
    return $fm
}

$Entries = @()
$ParseErrors = @()
foreach ($corpus in $Corpora) {
    $dir = Join-Path $KbPath $corpus
    if (-not (Test-Path -LiteralPath $dir)) { continue }
    foreach ($f in Get-ChildItem -LiteralPath $dir -Filter '*.md' -File) {
        try {
            $e = Read-KbEntry $f.FullName
            if ($null -eq $e) { $ParseErrors += $f.Name; continue }
            $status = 'active'; if ($e['status']) { $status = $e['status'] }
            if ($Statuses -contains $status) { $Entries += , $e }
        } catch { $ParseErrors += $f.Name }
    }
}
if ($Entries.Count -eq 0) { Write-KbWarning "no entries with status [$($Statuses -join ', ')] under $KbPath"; exit 0 }

# ---------- collect diff text per file ----------
function Get-DiffByFile([string[]]$GitArgs) {
    $map = [ordered]@{}
    $cur = $null
    $inHeader = $false
    $out = & git -c core.quotepath=off @GitArgs 2>$null
    foreach ($line in $out) {
        if ($line.StartsWith('diff --git ')) { $cur = $null; $inHeader = $true; continue }
        if ($inHeader -and $line.StartsWith('--- ')) {
            if ($line -ne '--- /dev/null') { $cur = $line.Substring(6) }
            continue
        }
        if ($inHeader -and $line.StartsWith('+++ ')) {
            if ($line -ne '+++ /dev/null') { $cur = $line.Substring(6) }
            if ($cur -and -not $map.Contains($cur)) { $map[$cur] = New-Object System.Text.StringBuilder }
            $inHeader = $false
            continue
        }
        if ($inHeader) { continue }
        if ($cur -and ($line.StartsWith('+') -or $line.StartsWith('-'))) {
            [void]$map[$cur].AppendLine($line.Substring(1))
        }
    }
    return $map
}

if ($Mode -eq 'commit') {
    $Diff = Get-DiffByFile @('show', $Commit, '--format=', '--unified=0', '--no-color', '--no-ext-diff')
    $Target = "commit $((& git rev-parse --short $Commit).Trim())"
} else {
    $Diff = Get-DiffByFile @('diff', 'HEAD', '--unified=0', '--no-color', '--no-ext-diff')
    $untracked = & git -c core.quotepath=off ls-files --others --exclude-standard 2>$null
    foreach ($u in $untracked) {
        $full = Join-Path $RepoRoot $u
        if (-not (Test-Path -LiteralPath $full -PathType Leaf)) { continue }
        if ((Get-Item -LiteralPath $full).Length -gt 2MB) { continue }
        $sb = New-Object System.Text.StringBuilder
        [void]$sb.Append([System.IO.File]::ReadAllText($full, $Utf8))
        $Diff[$u] = $sb
    }
    $Target = 'uncommitted changes'
}

# ---------- matching ----------
function Get-FileLanguages([string]$Path) {
    switch -Regex ([System.IO.Path]::GetExtension($Path).ToLowerInvariant()) {
        '^\.c$'                    { return @('c') }
        '^\.(cpp|cc|cxx|hpp|hh)$'  { return @('cpp') }
        '^\.h$'                    { return @('c', 'cpp') }
        '^\.cs$'                   { return @('csharp') }
        '^\.xaml$'                 { return @('xaml') }
        default                    { return @() }
    }
}

function Convert-GlobToRegex([string]$Glob) {
    $g = $Glob.Replace('\', '/')
    $sb = New-Object System.Text.StringBuilder
    $i = 0
    while ($i -lt $g.Length) {
        $c = $g[$i]
        if ($c -eq '*' -and $i + 1 -lt $g.Length -and $g[$i + 1] -eq '*') {
            if ($i + 2 -lt $g.Length -and $g[$i + 2] -eq '/') { [void]$sb.Append('(?:.*/)?'); $i += 3 }
            else { [void]$sb.Append('.*'); $i += 2 }
            continue
        }
        if ($c -eq '*') { [void]$sb.Append('[^/]*') }
        elseif ($c -eq '?') { [void]$sb.Append('[^/]') }
        else { [void]$sb.Append([regex]::Escape([string]$c)) }
        $i++
    }
    return '^' + $sb.ToString() + '$'
}

function Test-FilePattern([string]$Path, $Patterns) {
    $p = $Path.Replace('\', '/')
    $name = [System.IO.Path]::GetFileName($p)
    foreach ($glob in @($Patterns)) {
        if (-not $glob -or $glob -eq '*') { continue }
        $rx = Convert-GlobToRegex $glob
        if ($glob.Contains('/')) {
            if ($p -match $rx -or $p -match ('^(?:.*/)?' + $rx.Substring(1))) { return $true }
        } elseif ($name -match $rx) { return $true }
    }
    return $false
}

$Hits = @()
foreach ($file in $Diff.Keys) {
    $langs = Get-FileLanguages $file
    $text = $Diff[$file].ToString()
    if (-not $text) { continue }
    foreach ($e in $Entries) {
        $entryLangs = @($e['language'])
        $langOk = ($entryLangs -contains 'any')
        if (-not $langOk) { foreach ($l in $langs) { if ($entryLangs -contains $l) { $langOk = $true } } }
        if (-not $langOk) { continue }
        $kw = @()
        foreach ($k in @($e['code_keywords'])) {
            if ($k -and $text.Contains($k)) { $kw += $k }
        }
        if ($kw.Count -eq 0) { continue }
        $Hits += [pscustomobject]@{
            Id       = $e['id']
            Severity = $e['severity_anchor']
            Title    = $e['title']
            File     = $file
            Keywords = $kw
            Pattern  = (Test-FilePattern $file $e['file_pattern'])
            Entry    = $e['_path']
        }
    }
}

# ---------- output ----------
function Get-RelPath([string]$Path) {
    $root = $RepoRoot.Replace('/', '\').TrimEnd('\') + '\'
    $p = $Path.Replace('/', '\')
    if ($p.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) { return $p.Substring($root.Length).Replace('\', '/') }
    return $Path
}

$sevRank = @{ 'Critical' = 0; 'High' = 1; 'Medium' = 2; 'Low' = 3 }
$grouped = $Hits | Group-Object Id | Sort-Object @{ Expression = { $r = $sevRank[$_.Group[0].Severity]; if ($null -eq $r) { 9 } else { $r } } }, Name

if ($Json) {
    [pscustomobject]@{
        kbPath       = $KbPath
        kbVersion    = $KbVersion
        target       = $Target
        entries      = $Entries.Count
        filesScanned = $Diff.Count
        parseErrors  = $ParseErrors
        hits         = @($grouped | ForEach-Object {
            [pscustomobject]@{
                id       = $_.Name
                severity = $_.Group[0].Severity
                title    = $_.Group[0].Title
                entry    = (Get-RelPath $_.Group[0].Entry)
                files    = @($_.Group | ForEach-Object { [pscustomobject]@{ file = $_.File; keywords = $_.Keywords; patternMatch = $_.Pattern } })
            }
        })
    } | ConvertTo-Json -Depth 6
    exit 0
}

Write-Output "Security KB: $KbPath @ $KbVersion | target: $Target | active entries: $($Entries.Count) | files scanned: $($Diff.Count) | entries hit: $(@($grouped).Count)"
if ($ParseErrors.Count -gt 0) { Write-Output "Security KB: WARNING - could not parse: $($ParseErrors -join ', ')" }
if (@($grouped).Count -eq 0) { Write-Output 'No KB entry matched this diff.'; exit 0 }

Write-Output ''
Write-Output 'Candidate entries (Read each entry file and apply its ## Detection to the changed code; a hit is NOT a finding by itself):'
Write-Output ''
foreach ($g in $grouped) {
    $first = $g.Group[0]
    Write-Output "### $($g.Name) [$($first.Severity)] - $($first.Title)"
    Write-Output "Entry: $(Get-RelPath $first.Entry)"
    foreach ($h in $g.Group) {
        $pm = ''; if ($h.Pattern) { $pm = ' (file_pattern also matches)' }
        Write-Output "- $($h.File): $($h.Keywords -join ', ')$pm"
    }
    Write-Output ''
}
exit 0
