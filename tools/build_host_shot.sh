#!/bin/sh
# Ana makinede GLES (Mesa) ile ekran goruntusu aracini derler. /tmp/glinc: GLES/EGL basliklari.
set -e
R=$(cd "$(dirname "$0")/.." && pwd)
g++ -std=c++17 -O1 -DFS12_GL -I/tmp/glinc -I$R/src -I$R/third_party/box2d -w -o ${1:-/tmp/host_shot} $R/tools/host_shot.cpp \
  $R/src/render/*.cpp $R/src/game/GuiDeviceGL.cpp $R/src/game/MenuApp.cpp $R/src/game/GenericGUIManager.cpp $R/src/game/GuiUpdate.cpp \
  $R/src/game/ModBridge.cpp $R/src/game/SaveGames.cpp $R/src/game/WorldView.cpp $R/src/game/VehicleCatalog.cpp $R/src/game/GameState.cpp $R/src/game/MapData.cpp $R/src/game/MapPhysics.cpp $R/src/data/*.cpp $R/src/platform/SystemDevice.cpp $R/src/platform/InputDevice.cpp $R/src/platform/CryptoUtil.cpp $(find $R/third_party/box2d/Box2D -name '*.cpp') \
  /usr/lib/x86_64-linux-gnu/libGLESv2.so.2 /usr/lib/x86_64-linux-gnu/libEGL.so.1
