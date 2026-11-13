#!/bin/zsh
# Local uncooked preview using the installed Unreal Editor; this is not a packaged build.
set -eu
PROJECT_DIR=${0:A:h}
UE_EDITOR='/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor'
if [[ ! -x "$UE_EDITOR" ]]; then
  print -u2 'UE 5.8 editor not found. Open DirectorCutJam.uproject with your local UE installation.'
  exit 1
fi
exec "$UE_EDITOR" "$PROJECT_DIR/DirectorCutJam.uproject" /Game/Maps/Firefighter -game -windowed -ResX=1440 -ResY=900 -NoSplash "$@"
