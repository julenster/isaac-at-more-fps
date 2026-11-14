#!/bin/sh
# isaac-highfps installer for Linux / Steam Deck.
#   sh install-linux.sh            install
#   sh install-linux.sh uninstall  uninstall
HERE="$(cd "$(dirname "$0")" && pwd)"
GAME_DIR_NAME="The Binding of Isaac Rebirth"

find_game() {
    for root in "$HOME/.local/share/Steam" "$HOME/.steam/steam" "$HOME/.steam/root" \
                "$HOME/.var/app/com.valvesoftware.Steam/.local/share/Steam"; do
        vdf="$root/steamapps/libraryfolders.vdf"
        { echo "$root"; [ -f "$vdf" ] && sed -n 's/^[[:space:]]*"path"[[:space:]]*"\(.*\)"[[:space:]]*$/\1/p' "$vdf"; } |
        while IFS= read -r lib; do
            d="$lib/steamapps/common/$GAME_DIR_NAME"
            [ -f "$d/isaac-ng.exe" ] && { echo "$d"; return 0; }
        done | head -n 1
    done | head -n 1
}

echo
echo "  isaac-highfps"
echo
GAME="$(find_game)"
if [ -z "$GAME" ]; then
    echo "  Could not find Repentance+ (isaac-ng.exe)."
    echo "  Make sure the game is set to use Proton (Properties > Compatibility),"
    echo "  or copy winmm.dll manually into the game folder."
    exit 1
fi
echo "  Game folder: $GAME"

if pgrep -f "isaac-ng.exe" >/dev/null 2>&1; then
    echo "  The game is running. Close it and run this again."; exit 1
fi

if [ "$1" = "uninstall" ]; then
    rm -f "$GAME/winmm.dll" "$GAME/highfps.ini" "$GAME/highfps.log"
    echo "  Uninstalled. You can remove the launch option from Steam too."
    exit 0
fi

cp -f "$HERE/winmm.dll" "$GAME/winmm.dll"
[ -f "$GAME/highfps.ini" ] || cp "$HERE/highfps.ini" "$GAME/highfps.ini"
echo "  Installed!"
echo
echo "  One last step in Steam: right-click the game > Properties > General >"
echo "  Launch options, and paste this:"
echo
echo '      WINEDLLOVERRIDES="winmm=n,b" %command%'
echo
