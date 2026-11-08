// Coklu oyuncu modu ile oyun arasindaki DOGRUDAN baglanti noktalari (kanca/Substrate yok).
// Orijinal yamada MSHookFunction ile baglanan uc fonksiyonun yerini alir:
//   my_updateGUI            -> GenericGUIManager::update           (once/sonra)
//   my_renderMenu           -> GenericGUIManager::renderSaveGameMenu (once/sonra)
//   my_renderStartMenuMain  -> GenericGUIManager::renderStartMenuMain (sonra)
// Mod derlenmediginde (FS12_WITH_MOD=OFF) hepsi bos kalir ve oyun saf tek oyunculu calisir.
#pragma once
namespace fs12 {
class GenericGUIManager;
struct ModBridge {
    void (*updateBefore)(GenericGUIManager*) = nullptr;
    void (*updateAfter)(GenericGUIManager*) = nullptr;
    // Kayit menusu cizilmeden once: mod "Select a file" ekranina yeni girildi mi karar verir; baslik cizgisi yuksekligini dondurur
    void (*saveMenuBefore)(GenericGUIManager*) = nullptr;
    void (*saveMenuAfter)(GenericGUIManager*) = nullptr;
    void (*startMenuMainAfter)(GenericGUIManager*) = nullptr;
};
ModBridge& modBridge();
}
