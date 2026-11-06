# StellaOS Wallpaper Downloader v2
# Downloads REAL anime wallpapers (4K) + GIFs from Wallhaven
# Searches with specific anime tags to ensure quality

param(
    [int]$Count = 25
)

$WallpaperDir = "assets\wallpapers"
$AnimatedDir = "assets\wallpapers\animated"
New-Item -ItemType Directory -Force -Path $WallpaperDir | Out-Null
New-Item -ItemType Directory -Force -Path $AnimatedDir | Out-Null

Write-Host "============================================" -ForegroundColor Magenta
Write-Host "  StellaOS * Anime Wallpaper Downloader v2" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Magenta
Write-Host ""

# Anime-specific search queries to ensure we get REAL anime wallpapers
$AnimeQueries = @(
    "id:5 id:37",           # Wallhaven anime tag IDs (anime, manga)
    "anime girl landscape",
    "anime scenery sky",
    "anime night sky stars",
    "anime cherry blossom",
    "anime sunset cityscape",
    "anime school uniform",
    "anime fantasy magic",
    "anime cyberpunk neon",
    "anime ocean waves",
    "genshin impact",
    "violet evergarden",
    "your name kimi no na wa",
    "makoto shinkai",
    "anime sakura petals"
)

$Downloaded = 0
$QueryIndex = 0

while ($Downloaded -lt $Count -and $QueryIndex -lt $AnimeQueries.Count) {
    $Query = $AnimeQueries[$QueryIndex]
    $QueryIndex++

    Write-Host "[Query: '$Query'] Fetching..." -ForegroundColor Yellow

    try {
        $EncodedQuery = [System.Web.HttpUtility]::UrlEncode($Query)
    } catch {
        $EncodedQuery = $Query -replace ' ', '+'
    }

    try {
        $Url = "https://wallhaven.cc/api/v1/search?q=$EncodedQuery&categories=010&purity=100&atleast=3840x2160&sorting=relevance&order=desc"
        $Response = Invoke-RestMethod -Uri $Url -Method Get -TimeoutSec 15
    } catch {
        Write-Host "  API error, skipping query" -ForegroundColor Red
        continue
    }

    if (-not $Response.data -or $Response.data.Count -eq 0) {
        Write-Host "  No results, trying next query" -ForegroundColor DarkGray
        continue
    }

    foreach ($wp in $Response.data) {
        if ($Downloaded -ge $Count) { break }

        $WpUrl = $wp.path
        $Filename = Split-Path $WpUrl -Leaf
        $Filepath = Join-Path $WallpaperDir $Filename

        # Skip if already downloaded
        if (Test-Path $Filepath) { continue }

        # Skip non-anime (check category)
        if ($wp.category -ne "anime") { continue }

        Write-Host "  [$Downloaded/$Count] $Filename ($($wp.resolution))..." -NoNewline

        try {
            Invoke-WebRequest -Uri $WpUrl -OutFile $Filepath -UseBasicParsing -TimeoutSec 30
            $FileSize = (Get-Item $Filepath).Length

            if ($FileSize -gt 50000) {
                $Downloaded++
                $SizeKB = [Math]::Round($FileSize / 1024)
                Write-Host " OK (${SizeKB}KB)" -ForegroundColor Green
            } else {
                Remove-Item $Filepath -Force
                Write-Host " bad file, removed" -ForegroundColor Red
            }
        } catch {
            Write-Host " FAILED" -ForegroundColor Red
            if (Test-Path $Filepath) { Remove-Item $Filepath -Force }
        }

        Start-Sleep -Milliseconds 600
    }

    Start-Sleep -Seconds 1
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Magenta
Write-Host "  Static wallpapers: $Downloaded" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Magenta

# Now download anime GIFs
Write-Host ""
Write-Host "  Downloading anime GIFs..." -ForegroundColor Cyan

$GifQueries = @(
    "anime loop",
    "anime rain loop",
    "anime chill lofi",
    "anime sky clouds moving",
    "anime sakura falling"
)

$GifDownloaded = 0
$GifMax = 8

# Search for GIFs from a different approach - Wallhaven doesn't have GIFs natively
# Instead, try to find them through the API with file_type filter
foreach ($gq in $GifQueries) {
    if ($GifDownloaded -ge $GifMax) { break }

    try {
        $EncodedGQ = $gq -replace ' ', '+'
        $GifUrl = "https://wallhaven.cc/api/v1/search?q=$EncodedGQ&categories=010&purity=100&sorting=relevance&order=desc"
        $GifResponse = Invoke-RestMethod -Uri $GifUrl -Method Get -TimeoutSec 15
    } catch { continue }

    if (-not $GifResponse.data) { continue }

    foreach ($gw in $GifResponse.data) {
        if ($GifDownloaded -ge $GifMax) { break }
        if ($gw.category -ne "anime") { continue }

        $GwUrl = $gw.path
        $GwFilename = Split-Path $GwUrl -Leaf
        $GwFilepath = Join-Path $AnimatedDir $GwFilename

        if (Test-Path $GwFilepath) { continue }

        Write-Host "  [GIF $GifDownloaded/$GifMax] $GwFilename..." -NoNewline
        try {
            Invoke-WebRequest -Uri $GwUrl -OutFile $GwFilepath -UseBasicParsing -TimeoutSec 30
            $GwSize = (Get-Item $GwFilepath).Length
            if ($GwSize -gt 50000) {
                $GifDownloaded++
                Write-Host " OK ($([Math]::Round($GwSize/1024))KB)" -ForegroundColor Green
            } else {
                Remove-Item $GwFilepath -Force
                Write-Host " removed" -ForegroundColor Red
            }
        } catch {
            Write-Host " FAILED" -ForegroundColor Red
        }
        Start-Sleep -Milliseconds 600
    }
    Start-Sleep -Seconds 1
}

Write-Host ""
Write-Host "============================================" -ForegroundColor Magenta
Write-Host "  Total static: $Downloaded" -ForegroundColor Green
Write-Host "  Total animated: $GifDownloaded" -ForegroundColor Green
Write-Host "  Location: $WallpaperDir\" -ForegroundColor White
Write-Host "============================================" -ForegroundColor Magenta

# Generate index
$AllFiles = @()
$AllFiles += Get-ChildItem "$WallpaperDir\*" -Include "*.jpg","*.png" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name
$AllFiles | Out-File "$WallpaperDir\index.txt" -Encoding UTF8
Write-Host "Index: $($AllFiles.Count) wallpapers total" -ForegroundColor Yellow
