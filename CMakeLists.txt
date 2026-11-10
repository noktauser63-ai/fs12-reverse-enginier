cmake_minimum_required(VERSION 3.22)
project(FS12Rebuild LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# ARM softfp ile ayni float sonuclari icin FMA birlestirme kapali (arm64/x86 farki olmasin)
add_compile_options(-ffp-contract=off)

option(FS12_WITH_MOD "Coklu oyuncu modunu (ImGui + ag) derle" OFF)   # Android CI once ON dener, olmazsa OFF
option(FS12_BUILD_TESTS "Ana makinede testleri derle" OFF)

# ---- Box2D 2.1.2 (oyundaki sürümün birebir kaynağı; b2_version = {2,1,2}) ----
file(GLOB_RECURSE BOX2D_SRC third_party/box2d/Box2D/*.cpp)
add_library(box2d STATIC ${BOX2D_SRC})
target_include_directories(box2d PUBLIC third_party/box2d)
target_compile_options(box2d PRIVATE -w)

# ---- Motor cekirdegi (matematik, yardimcilar) ----
file(GLOB ENGINE_SRC src/engine/*.cpp)
add_library(fs12_engine STATIC ${ENGINE_SRC})
target_include_directories(fs12_engine PUBLIC src)
target_link_libraries(fs12_engine PUBLIC box2d fs12_audio)

# ---- Oyun verisi (orijinal libapp.so'dan cikarilan tablolar) + font yerlesimi ----
add_library(fs12_data STATIC src/data/Strings.cpp src/data/FontData03.cpp src/data/GuiStatics.cpp src/data/GuiInitialState.cpp src/data/Credits.cpp src/data/StringsTr.cpp src/data/FontTr.cpp src/render/Font.cpp src/render/P2D.cpp)

# ---- Oyun mantigi (GUI, ...) ----
add_library(fs12_game STATIC src/game/GenericGUIManager.cpp src/game/GuiUpdate.cpp src/game/ModBridge.cpp src/game/MenuApp.cpp src/game/SaveGames.cpp src/game/MapData.cpp src/game/Cowshed.cpp src/game/CowHerd.cpp)
target_link_libraries(fs12_game PUBLIC fs12_data fs12_platform fs12_engine)
target_include_directories(fs12_data PUBLIC src)

# ---- Platform katmani (dosya/asset, kayit sifreleme, girdi) ----
file(GLOB PLATFORM_SRC src/platform/*.cpp)
# Activity/entry native_app_glue ister; yalniz "app" kutuphanesinde derlenir
list(FILTER PLATFORM_SRC EXCLUDE REGEX "(AndroidActivity|android_entry)\\.cpp$")
add_library(fs12_platform STATIC ${PLATFORM_SRC})
target_include_directories(fs12_platform PUBLIC src)

# ---- Ses motoru (FMOD yerine) ----
add_library(fs12_audio STATIC
    src/audio/engine.cpp
    src/audio/p1d.cpp
    src/audio/output_null.cpp
    src/audio/output_opensl.cpp
    src/audio/stb_vorbis_impl.c)
target_include_directories(fs12_audio PUBLIC src)
set_source_files_properties(src/audio/stb_vorbis_impl.c PROPERTIES COMPILE_FLAGS "-w")

if(ANDROID)
    # Android 15+ (SDK 35) 16 KB sayfa boyutu uyumu
    add_link_options(-Wl,-z,max-page-size=16384)
    target_link_libraries(fs12_audio PUBLIC OpenSLES log)
    # Asama 1 dogrulama kutuphanesi: tum ABI'lerde ses motoru derlenip baglaniyor mu?
    # ---- NativeActivity uygulamasi (platform duman testi): libapp.so ----
    set(GLUE_DIR ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
    add_library(app SHARED ${GLUE_DIR}/android_native_app_glue.c src/platform/AndroidActivity.cpp src/platform/android_entry.cpp
        src/render/Renderer.cpp src/game/GuiDeviceGL.cpp src/game/WorldView.cpp)
    target_link_libraries(app PRIVATE fs12_data fs12_game)
    target_include_directories(app PRIVATE ${GLUE_DIR})
    target_link_libraries(app PRIVATE fs12_platform fs12_audio android EGL GLESv2 log)
    set_target_properties(app PROPERTIES LINK_FLAGS "-u ANativeActivity_onCreate")
    if(FS12_WITH_MOD)
        # Cok oyunculu mod (ImGui + ag): kancasiz, dogrudan baglanti (src/mod, src/game/ModBridge.h)
        add_library(fs12_mod STATIC
            src/mod/multiplayer.cpp
            third_party/imgui/imgui.cpp third_party/imgui/imgui_draw.cpp
            third_party/imgui/imgui_tables.cpp third_party/imgui/imgui_widgets.cpp
            third_party/imgui/backends/imgui_impl_opengl3.cpp)
        target_compile_definitions(fs12_mod PUBLIC IMGUI_IMPL_OPENGL_ES2 FS12_WITH_MOD)
        target_compile_options(fs12_mod PRIVATE -w)
        target_include_directories(fs12_mod PUBLIC src third_party third_party/imgui third_party/imgui/backends)
        target_link_libraries(fs12_mod PUBLIC fs12_game android log EGL GLESv2)
        target_link_libraries(app PRIVATE fs12_mod)
        target_compile_definitions(app PRIVATE FS12_WITH_MOD)
        target_include_directories(app PRIVATE src)
    endif()
    add_library(fs12audio SHARED src/audio/jni_audio_probe.cpp)
    target_link_libraries(fs12audio PRIVATE fs12_audio)
endif()

if(FS12_BUILD_TESTS AND NOT ANDROID)
    add_executable(audio_test tests/audio_test.cpp)
    target_link_libraries(audio_test PRIVATE fs12_audio)
    target_compile_options(audio_test PRIVATE -fsanitize=address,undefined -g)
    target_link_options(audio_test PRIVATE -fsanitize=address,undefined)
    add_executable(math_test tests/math_test.cpp)
    target_link_libraries(math_test PRIVATE fs12_engine)
    target_compile_options(math_test PRIVATE -fsanitize=address,undefined -g)
    target_link_options(math_test PRIVATE -fsanitize=address,undefined)
    add_executable(platform_test tests/platform_test.cpp)
    target_link_libraries(platform_test PRIVATE fs12_platform)
    target_compile_options(platform_test PRIVATE -fsanitize=address,undefined -g)
    target_link_options(platform_test PRIVATE -fsanitize=address,undefined)
    add_executable(gui_trace_test tests/gui_trace_test.cpp)
    target_link_libraries(gui_trace_test PRIVATE fs12_game)
    add_executable(gui_input_test tests/gui_input_test.cpp)
    target_link_libraries(gui_input_test PRIVATE fs12_game fs12_platform)
    add_executable(menu_flow_test tests/menu_flow_test.cpp)
    target_link_libraries(menu_flow_test PRIVATE fs12_game fs12_platform)
    target_include_directories(menu_flow_test PRIVATE src)
    add_executable(savegames_test tests/savegames_test.cpp)
    target_link_libraries(savegames_test PRIVATE fs12_game fs12_platform)
    target_include_directories(savegames_test PRIVATE src)
    add_executable(mapdata_test tests/mapdata_test.cpp)
    target_link_libraries(mapdata_test PRIVATE fs12_game fs12_platform)
    target_include_directories(mapdata_test PRIVATE src)
    add_executable(cowherd_test tests/cowherd_test.cpp)
    target_link_libraries(cowherd_test PRIVATE fs12_game fs12_engine fs12_platform)
    target_include_directories(cowherd_test PRIVATE src)
    add_executable(cowshed_test tests/cowshed_test.cpp)
    target_link_libraries(cowshed_test PRIVATE fs12_game fs12_platform)
    target_include_directories(cowshed_test PRIVATE src)
    add_executable(layout_test tests/layout_test.cpp)
    target_include_directories(layout_test PRIVATE src)
    add_executable(font_test tests/font_test.cpp)
    target_link_libraries(font_test PRIVATE fs12_data)
    add_executable(p2d_dump tests/p2d_dump.cpp)
    target_link_libraries(p2d_dump PRIVATE fs12_data)
    target_include_directories(p2d_dump PRIVATE src)
    add_executable(physics_test tests/physics_test.cpp)
    target_link_libraries(physics_test PRIVATE box2d)
endif()
