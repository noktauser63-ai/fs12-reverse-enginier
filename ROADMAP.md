# FS12 libapp.so yeniden inşa — yol haritası (canlı belge)

Her oturum başında bu dosyayı oku, sonunda "Durum" bölümünü güncelle.

## Bulgular (2026-11-02)
- libapp.so: 32-bit ARM, ARMv5TE, ~980 KB, 2010 dışa aktarılan sembol (mangled isimler = sınıf/imza kaynağı).
- Bağımlılıklar: libfmodex.so (FMOD, kaldırılacak), GLESv2, EGL, android, log, stdc++.
- Decompile (libapp.so.c): ~119k satır, ~2146 fonksiyon, ~610 isimsiz FUN_. Sınıf struct'ları BOŞ (placeholder),
  alan düzeni yok → offset'lerden (`this+0x44`) elle çıkarılmalı.
- Decompile hatalı örnekler var: `AudioSource::play` içinde `this` yerine `param_3` kullanılmış (ARM32 çağrı
  kuralı karışmış). Fonksiyonlar derlenmeden önce disassembly/mangled imza ile doğrulanmalı.
- Kod payı: Box2D (b2*) ≈ 1/4. Oyundaki sürüm Box2D **2.1.2** (b2_version okundu). Resmi kaynak (erincatto/box2d commit 8a34286, stripped dosyalar blob id'den kurtarıldı) third_party/box2d altında; decompile'dan çevrilmeyecek.
- Satın alma: Google Play yok. IAP cihaz sınıfı arayüz olarak kalır, boş (NullPurchaseDevice) uygulanır; manifestten BILLING/CHECK_LICENSE izinleri çıkarılır.
- tools/split_decompile.py: decompile'ı sınıf bazlı parçalar (work/decomp/<Sınıf>.c + index.tsv). Gerçek oyun kodu: ~35k satır adlı sınıf + ~777 isimsiz FUN_ (çoğu libstdc++ şablonu).
- Ses: .p1d = 8 baytlık başlık (u32 tür: 2 = RIFF/WAV [IMA-ADPCM 0x11 veya PCM], 5 = OggVorbis) + ham dosya.
  Müzik = Ogg Vorbis (music01-05), efektler = WAV. FMOD yerine OpenSL ES/AAudio + stb_vorbis + IMA-ADPCM çözücü yeterli.
- Çıktı hedefi: yeni libapp.so (hook yok), yeni.cpp içine gömülü multiplayer, buton_texture.h gömülü.

## Aşamalar
1. Temel: sınıf başlıkları (struct düzenleri), `engine/` iskeleti, CMake (NDK sürümü kararı bekliyor), CI.
2. Ses motoru (sıfırdan): p1d yükleyici, ADPCM/Vorbis çözücü, mixer, AudioSource/AudioBuffer/BackgroundMusicPlayer/VehicleSound.
3. Matematik/yardımcılar: Vector3, Matrix4x4, Quaternion, MathUtil, RandomGenerator, BitVector, CubicSpline2.
4. Box2D (upstream) + Game::DynamicsOverlapCallback vb. köprüler.
5. Render/GUI: GLESHandheldRenderDevice, GenericGUIManager, Plane, Light, Environment.
6. Oyun mantığı: Game, Vehicle, Trailer, Tool, Map/Field/GridCell, MissionManager, TipSite, SaveGames, Bale, Traffic.
7. Platform: AndroidActivity, JNICall, input, Moga, IAP (stub).
8. Multiplayer: yeni.cpp'deki hook'ları doğrudan çağrıya çevir, ImGui + buton_texture.h gömme.
9. Doğrulama: orijinal libapp.so ile fonksiyon bazlı karşılaştırma (saf fonksiyonlar), cihazda test.

## Kararlar (kullanıcıdan)
- Hedef: arm64-v8a build + evrensel APK (arm64-v8a, armeabi-v7a, x86_64 [MuMu emülatörü için]), SDK 35/36, modern NDK, 16 KB sayfa hizası.
- Ghidra erişimi yok; elde yalnızca libapp.so.c (+ APK içindeki libapp.so ikilisi: nm/objdump ile imza/disassembly doğrulaması yapılabilir).
- CI hatası (kullanıcının platformu): android-actions/setup-android@v3 `sdkmanager tools` çağırıyor, paket artık yok.
  Çözüm: eylemde `packages: ''` ver ya da (bizim workflow'da olduğu gibi) eylemi hiç kullanma, runner'ın hazır SDK/NDK'sını kullan.

## Açık sorular
- Java tarafı (classes*.dex) hazır APK'dan alınacak; AndroidManifest SDK 35 için yamalanmalı (android:exported, targetSdk). Aşama 7'de.

## Durum
- [x] Zip ve decompile incelendi, ses formatı çözüldü.
- [x] Aşama 2 (ses motoru çekirdeği): p1d, IMA-ADPCM (30 WAV ffmpeg ile bit-bit aynı), Ogg müzik akışı, mixer, OpenSL ES çıkışı. Host testi geçti.
- [x] CMake + rebuild.yml (host test + 3 ABI). Android tarafı bu ortamda derlenemedi (NDK indirilemiyor) → ilk CI koşusu doğrulayacak.
- [x] Box2D 2.1.2 projeye alındı, fizik duman testi geçti.
- [x] Aşama 3 (matematik): RandomGenerator, BitVector, Vector3/4, Matrix4x4, Quaternion, Plane, MathUtil(+Handheld), CubicSpline2 — assembly ile doğrulandı, tests/math_test.cpp geçti.
  Araçlar: tools/disasm.py <libapp.so> '<sınıf::fonksiyon>' (Thumb asm), tools/split_decompile.py.
- [ ] Sıradaki (eski not): AudioBuffer/AudioSource/VehicleSound/BackgroundMusicPlayer sınıflarını oyun arayüzüne bağlamak, sonra Aşama 1 (yapı başlıkları) + Aşama 3 (matematik).


## Kural notları (her oturumda uygula)
- Float işlem sırası orijinalle aynı; softfp decompile'ında kaybolan operandlar için disasm.py'ye bak.
- Sınıf adları global (namespace yok) — yeni.cpp ile doğrudan bağlanacak.
- Kayıt dosyası (SaveGames/Game::serialize) formatı bayt bayt aynı kalmalı.
- Satın alma: NullPurchaseDevice (mağaza yok, her şey kapalı/para paketleri gizli).

## Durum (oturum 4 — platform katmani)
- [x] src/platform: CryptoUtil (RC4+CRC32, kayit sifreleme: 16B onek + RC4(deviceID+onek)), SystemDevice (asset/kayit/dil/log; host + Android), InputDevice (dokunma tablosu, gamepad maskeleri, egim), PurchaseDevice (bos; IAP kaldirildi), lisans=true, bulut yedek=no-op
- [x] AndroidActivity (EGL config secimi orijinaldeki gibi, ES2 context, giris yonlendirme), android_entry.cpp (duman testi: music01 + dokunma rengi)
- [x] tools/make_apk.sh + packaging/AndroidManifest.xml (SDK35, BILLING izni yok, 16 KB hiza, imza) + workflow `package` isi: FS12-smoke-arm64.apk ve FS12-smoke-universal.apk
- DOGRULANMADI: Android'e ozel kod (AndroidActivity.cpp, android_entry.cpp, output_opensl.cpp) ilk CI'da derlenecek; APK cihazda denenmedi.
- SIRADA: ses sinif katmani (AudioBuffer/AudioSource/BackgroundMusicPlayer/VehicleSound), sonra render cihazi (GLESHandheldRenderDevice), GUI, oyun mantigi.

## Durum (oturum 5 — render temeli)
- Cihazda dogrulandi (kullanici): platform + EGL + muzik calisiyor.
- [x] .p2d formati cozuldu: 20B baslik [fmt,w,h,ekstraMip,bayrak]; fmt 0=RGBA8, 4=RGB565, 6=L8, 12=ETC1 (+1,2,3,5,7,8-11,13 tablo); son 1x1 mip dosyada 7 bayt kisa (padBytes). 63 dosya gorsel dogrulandi (resimler ters/yukari-asagi: overlay v'yi cevirir).
- [x] .p3d mesh: [u32 vert][u32 indis][vert*24: pos f3, normal 4xint8, uv f2][u16 indis][tek ise +2 dolgu].
- [x] src/render: P2D (parser + yazilim ETC1), Renderer (shader onbellegi, texture, mesh, 2D overlay; context kaybi/geri yukleme).
- Shader'lar orijinal assets/shader/*.vsh|fsh dosyalarindan okunur (attrib: Position=0, Normal=1, TexCoord0=2).
- SIRADA: yazi tipi (font03_P L8 atlas + FontInfo), GenericGUIManager (baslik ekrani/menu), LocalizationStringManager, sonra .m3d harita + dunya cizimi.

## Durum (oturum 6 — GUI, oracle ile dogrulanmis)
Yontem: orijinal libapp.so Unicorn'da calistirilir (tools/oracle.py), GL/overlay cagrilari ve durum degisimleri "altin" dosyalara
yazilir (tests/golden), C++ portu bit-bit karsilastirilir. Dekompile float argumanlarini kaybeder; kesin degerler icin
`python3 tools/dis2.py libapp.so '<ad>'` (literal havuzu + pc-goreli dizeleri cozer) kullanilir.

Tamam (altin izle 0 fark): GuiScale/anchorShift, renderMainMenu, renderTitle, renderTitleScreen(float), renderStartMenuMain,
renderStartMenuSettings, renderStartMenuHelp, renderCreditsMenu, renderSaveGameMenu (+formatNumber), renderGuiButtons,
updateGuiButtons (108 dugme x dokunma/surukleme/gamepad; tests/golden/gui_input.txt).
Not: orijinal ikilide updateGuiButtons 0x20/0x21 durumunda slot indeksini bayt ofseti gibi okur — aynen korundu.
Yazildi, henuz oracle ile dogrulanmadi: update(), updateMainMenu/CreditsMenu/SaveGameMenu/StartMenuHelp/Dialog.
Sirada: renderStartMenuPrices/Statistics/Shop, renderDialog, renderBaseMenu, updateBaseMenu, Game durum makinesi.
CI host-tests: layout_test, gui_trace_test, gui_input_test.

## Cok oyunculu mod entegrasyonu (oturum 6)
- Kaynak: src/mod/multiplayer.cpp (yeni.cpp'den uretildi; fark: tools degil, bkz. src/mod/ModApi.h, SubstrateOff.h). Orijinal: multiplayer_orig.cpp.
- Kanca YOK: MSHookFunction yerine src/game/ModBridge.h kopruleri: GenericGUIManager::update (once/sonra),
  renderSaveGameMenu (once/sonra), renderStartMenuMain (sonra). Girdi: AndroidActivity::handleInput -> ModFilterInputEvent. Baslangic: ModInit().
- Aktif: menu katmani (mod secimi Offline/LAN/Server, oda listesi, takma ad, sohbet, klavye), ag (LAN kesfi, TCP/UDP), ImGui cizimi.
- Pasif (derlenir, cagrilmaz): oyun-ici esitleme (arac/romork/alet/tarla/yaya/para). Bunlar 32-bit ham ofsetlerle orijinal nesnelere
  bakar; Game/Vehicle/Tool/Trailer/Map sinifleri 64-bit yeniden yazilirken `#ifdef FS12_MOD_GAME_SYNC` bolumleri yeni yapilara baglanacak.
- Derleme: -DFS12_WITH_MOD=ON (CI'da `android-mod` isi, arm64+armv7, continue-on-error).

## Durum (oturum 7 — menuler ekrana bagli)
- [x] GuiDeviceGL (src/game): GuiRenderDevice -> Renderer. Yazi: orijinal renderText mantigi (dikey bit4 orta/bit5 alt, yatay bit1 orta/bit2 sag,
  '\n' = 1.1 satir, olcek = boyut/64 * aspect olcegi). Overlay: Renderer::overlay orijinalin (u,1-v,du,-dv) kodlamasini zaten kullanir (donusum yok).
  Doku kimlikleri: GuiDeviceGL::TexId (font, elements, shopAlpha, shopBg, gestures, map, title, logo_<dil>, edition).
- [x] GenericGUIManager::render/renderStartMenu/renderDifficultyLevelMenu, updateStartMenu(+Main/Settings), updateDifficultyLevelMenu (orijinal dispatch: sayfa = this+0x58cc).
- [x] data/Credits: orijinal 52 satirlik kredi dizisi (GUIInterfaceDesc ctor); FMOD satiri yeni ses motoruyla degistirildi.
- [x] MenuApp: ana menu(2) -> kayit(4)/kredi(3) -> zorluk(5) -> baslangic menusu(7, oyun yer tutucusu). tests/menu_flow_test (host).
- android_entry artik MenuApp'i calistirir (duman testi kalkti). Cihazda DOGRULANMADI (GL cizimi, yazi yerlesimi).
- Sirada: renderStartMenuPrices/Statistics/Shop, renderDialog, Game durum makinesi (savegame yukleme/yeni oyun), dunya cizimi.

## Dil: Turkce + Ingilizce (oturum 7)
- Dil kodu "tr" -> LANG_TR (10); cihaz dili Turkce ise Turkce, bilinmeyen dillerde Ingilizce. Diger orijinal diller (de, fr, ja, zh, ru, es, it, pt, ko) duruyor.
- data/StringsTr.cpp (tools/gen_strings_tr.py'den uretilir, 312 dize; %s ve satir sirasi orijinalle ayni indekste).
- Font atlasinda Turkce harfler yok (Ç ğ Ğ ı İ ş Ş Ü). tools/patch_font_tr.py bunlari mevcut glif parcalarindan (ç cedilla, Ö/ö noktalari, i noktasi, cizilmis breve) uretir; CI paketlemede font03_P.p2d yamalanir,
  glif tablosu data/FontTr.cpp (ayni araca ait cikti, CI diff ile dogrular). Cihazda gorsel kontrol bekliyor.
- Mod (ImGui) menu metinleri henuz Ingilizce/Turkce karisik; yerellestirme sirada.

## Durum (oturum 8 — istatistik, fiyatlar, market)
- renderStartMenuStatistics / Prices / Shop: orijinalle bit-bit dogrulandi (gui_trace_test 25 senaryo, 0 fark; oracle'a cosf/sinf host libm ile eklendi).
- updateStartMenuShop: decompile'dan portlandi (karusel suruklemesi + atalet/yay). Oracle ile DOGRULANMADI (dokunma kuklasi gerekir).
- InputDevice::Touch.prevTime eklendi (orijinal +0x30).
- GuiDeviceGL: setNumberMonospace (rakam adimi = w/480*s0) uygulandi; Font::layoutXY/widthXY mono parametresi.
- Market satin alma geri cagrisi no-op (Google Play kaldirildi). GuiShopCtl alanlari Game gelince doldurulacak.
- Siradaki: renderDialog, taban menusu (durum 6), oyun durum makinesi, mod menulerinin TR/EN yerellestirmesi.

## Durum (oturum 9 — diyalog, kayitlar)
- renderDialog: orijinalle bit-bit dogrulandi (gui_trace_test 29 senaryo, 0 fark). GuiDialogCtl (id/metin/satir sayisi/yuva adlari).
- SaveGames (src/game/SaveGames.*): master.dat (0x5c bayt, crc32), save01-03.dat, tempSave.dat, gorevler 0..7; eski 0x4c bicim; eszamanli calisir. tests/savegames_test.cpp.
- MenuApp: kayit menusu orijinal updateStateSavegame mantigiyla (yuva: dolu->yukle, bos->zorluk; sil->onay diyalogu); zorluk -> CMD_NEW.
- Siradaki: renderBaseMenu/updateBaseMenu (GUI durum 6, oyun ici HUD), Game::init/update durum makinesi, Map/EntityManager.

## Oturum 10
- MapData (m3d ayristirici) + WorldView (harita 3B gorunumu) + WorldCamera (dokunmatik serbest kamera).
- MenuApp: startGame/pauseGame/inGame; yeni oyun/yukleme -> dunya; geri tusu veya sag ust kose -> duraklat menusu; geri = devam, cikis = baslik.
- Buycoin dugmesi gizlendi (Google Play kaldirildi). tools/host_shot ile ekran goruntusu dogrulamasi.
- Siradaki: HUD (renderBaseMenu), arazi/tarla katmanlari, arac/varlik sistemi.

## Oturum 11 — Ahir (Cowshed) modulu
- Kaynak: FS14 libfs14.so, Cows::update @ 0024e268 (Ghidra decompile). Sabitler: yem 6000 L/gun, saman 4000 L/gun, gubre 1000 L/gun,
  sagim 06-07 ve 17-18 arasi (potansiyel -3000/saat, tanka en fazla 1000 L/saat), sut tanki 14000 L, ahir 32 ot + 32 saman balyasi.
- [x] src/game/Cowshed.{h,cpp}: saf mantik (Game bagimliligi yok), kayit (serialize/deserialize v1), tests/cowshed_test.cpp (host, gecti).
- [ ] TODO(constants.txt): BALE_VOLUME, MAX_MANURE_FILL_LEVEL, DAY_START_HOUR/NIGHT_START_HOUR, BALE_PRICE degerleri (FS14 .so veri bolumu; DumpConstants.java ile alinacak).
- [ ] Game/Map/TipSite/Tool sinifi yok: "kapali ambar (saman satisi)" nesnesi haritada (m3d) duruyor, kodda karsiligi henuz yok.
      Ambar -> ahir degisimi icin: harita nesnesi + satis noktasi mantigi Game portuyla birlikte yapilacak.
- [ ] Gorsel: cowStanding/cowSleeping .p3d (+_shadow), cowStanding(02)_diffuse.p2d, bale.p3d, bale_grass/bale_straw.p2d. Renderer mesh/doku yukleyebiliyor; COW_ANIM shader'i (FS14) ayri portlanacak.
- Not: ekonomi degerleri (ahir 120.000, sut 0,60/L) ONERI; oynayarak ayarlanacak.

## Oturum 12 — inekler
- [x] tools/convert_p3d_fs14_to_fs12.py (FS14 28 bayt vertex + surum alani -> FS12 24 bayt). assets_extra/objects: cevrilmis inek/balya/gubre dosyalari; rebuild.yml paketlemede assets'e kopyalar.
- [x] src/game/CowHerd.* (7 inek yerlesimi, gece yatma), WorldView::setCowHerd/renderCows (GL'de DOGRULANMADI), tests/cowherd_test.cpp (gecti).
- [x] FS12 TipSite/Map::load analizi: docs/AHIR_ENTEGRASYON.md (7 satis/depo/BGA noktasi, fiyat formulu).
- Siradaki: ambar noktasini belirle, Game baglantisi, COW_ANIM.
- [x] Ahir yeri: silolarin karsisindaki kapali ambar = harita nesnesi #92 (102.3, 199.8); WorldView::hideObject, CowshedSite.h, android_entry baglandi (GL/cihazda dogrulanmadi). tools/map_objects.cpp.
