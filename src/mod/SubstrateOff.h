// Kancasiz derleme: orijinal yama MSHookFunction ile oyun fonksiyonlarini degistiriyordu.
// Yeniden yazilan oyunda ilgili noktalar dogrudan cagri/koprudur; bu golge yalnizca eski (pasif) kod yollarini derlenebilir tutar.
#pragma once
static inline void MSHookFunction(void*, void*, void** original) { if (original) *original = nullptr; }
