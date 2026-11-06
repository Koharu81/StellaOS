#!/bin/bash
# StellaOS Animated Wallpaper Downloader
# Downloads anime GIFs and short clips for animated wallpaper support
# Usage: bash scripts/download_animated_wallpapers.sh [count]

set -e

ANIMATED_DIR="assets/wallpapers/animated"
COUNT=${1:-10}

mkdir -p "$ANIMATED_DIR"

echo "============================================"
echo "  StellaOS ★ Animated Wallpaper Downloader"
echo "  Downloading ${COUNT} animated wallpapers..."
echo "============================================"
echo ""

DOWNLOADED=0

# Fetch from Wallhaven with type=png (which includes animated)
for PAGE in 1 2 3 4 5; do
    if [ $DOWNLOADED -ge $COUNT ]; then
        break
    fi

    echo "[Page ${PAGE}] Fetching animated wallpapers..."

    RESPONSE=$(curl -s "https://wallhaven.cc/api/v1/search?categories=010&purity=100&sorting=random&page=${PAGE}&ratios=16x9")

    PATHS=$(echo "$RESPONSE" | grep -o '"path":"[^"]*"' | sed 's/"path":"//g' | sed 's/"//g')

    for URL in $PATHS; do
        if [ $DOWNLOADED -ge $COUNT ]; then
            break
        fi

        FILENAME=$(basename "$URL")
        FILEPATH="${ANIMATED_DIR}/${FILENAME}"

        echo "  [${DOWNLOADED}/${COUNT}] Downloading ${FILENAME}..."
        if curl -s -o "$FILEPATH" "$URL"; then
            FILESIZE=$(stat -f%z "$FILEPATH" 2>/dev/null || stat -c%s "$FILEPATH" 2>/dev/null || echo "0")
            if [ "$FILESIZE" -gt 10000 ]; then
                DOWNLOADED=$((DOWNLOADED + 1))
                echo "    ✓ OK ($(( FILESIZE / 1024 ))KB)"
            else
                rm -f "$FILEPATH"
            fi
        fi
        sleep 0.5
    done
    sleep 1
done

echo ""
echo "Downloaded: ${DOWNLOADED} animated wallpapers"
echo "Location: ${ANIMATED_DIR}/"

