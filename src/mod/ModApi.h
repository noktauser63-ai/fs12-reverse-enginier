// Cok oyunculu modun oyuna acilan dogrudan arayuzu (kanca yok).
#pragma once
struct AInputEvent;
void ModInit();                          // acilista bir kez
bool ModFilterInputEvent(AInputEvent*);  // true = olay modca yutuldu (ImGui / klavye / dokunma)
float* fs12ModTitleUnderline();          // g_m_overlayTitleUnderlineRight (float[6]); [1] = yukseklik olcegi
