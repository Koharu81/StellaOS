#!/bin/bash
# StellaOS Wallpaper Downloader
# Downloads 4K anime wallpapers from Wallhaven API (free, no API key needed for SFW)
# Usage: bash scripts/download_wallpapers.sh [count]

set -e

WALLPAPER_DIR="assets/wallpapers"
COUNT=${1:-20}
PAGE_SIZE=24
TOTAL_PAGES=$(( (COUNT + PAGE_SIZE - 1) / PAGE_SIZE ))

mkdir -p "$WALLPAPER_DIR"

echo "============================================"
echo "  StellaOS ★ Wallpaper Downloader"
echo "  Downloading ${COUNT} 4K anime wallpapers..."
echo "============================================"
echo ""

DOWNLOADED=0

for PAGE in $(seq 1 $TOTAL_PAGES); do
    if [ $DOWNLOADED -ge $COUNT ]; then
        break
    fi

    echo "[Page ${PAGE}] Fetching wallpaper list..."

    # Search for 4K SFW anime wallpapers, sorted randomly
    RESPONSE=$(curl -s "https://wallhaven.cc/api/v1/search?categories=010&purity=100&atleast=3840x2160&sorting=random&page=${PAGE}")

    # Parse JSON to extract wallpaper paths (using grep/sed for portability)
    PATHS=$(echo "$RESPONSE" | grep -o '"path":"[^"]*"' | sed 's/"path":"//g' | sed 's/"//g')

    if [ -z "$PATHS" ]; then
        echo "  No wallpapers found on page ${PAGE}, trying next..."
        continue
    fi

    for URL in $PATHS; do
        if [ $DOWNLOADED -ge $COUNT ]; then
            break
        fi

        FILENAME=$(basename "$URL")
        FILEPATH="${WALLPAPER_DIR}/${FILENAME}"

        if [ -f "$FILEPATH" ]; then
            echo "  [SKIP] ${FILENAME} (already exists)"
            continue
        fi

        echo "  [${DOWNLOADED}/${COUNT}] Downloading ${FILENAME}..."
        if curl -s -o "$FILEPATH" "$URL"; then
            # Verify file is actually an image (at least 10KB)
            FILESIZE=$(stat -f%z "$FILEPATH" 2>/dev/null || stat -c%s "$FILEPATH" 2>/dev/null || echo "0")
            if [ "$FILESIZE" -gt 10000 ]; then
                DOWNLOADED=$((DOWNLOADED + 1))
                echo "    ✓ OK ($(( FILESIZE / 1024 ))KB)"
            else
                echo "    ✗ File too small, removing..."
                rm -f "$FILEPATH"
            fi
        else
            echo "    ✗ Download failed"
        fi

        # Rate limiting - be nice to the API
        sleep 0.5
    done

    sleep 1
done

echo ""
echo "============================================"
echo "  Download complete!"
echo "  Downloaded: ${DOWNLOADED} wallpapers"
echo "  Location: ${WALLPAPER_DIR}/"
echo "============================================"

# Generate wallpaper index file
echo "Generating wallpaper index..."
ls -1 "$WALLPAPER_DIR"/*.jpg "$WALLPAPER_DIR"/*.png 2>/dev/null | while read f; do
    basename "$f"
done > "${WALLPAPER_DIR}/index.txt"

echo "Index written to ${WALLPAPER_DIR}/index.txt"
echo "Total wallpapers: $(wc -l < "${WALLPAPER_DIR}/index.txt" 2>/dev/null || echo 0)"

