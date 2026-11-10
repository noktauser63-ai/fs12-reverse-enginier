# Ahir entegrasyonu — durum ve bulgular

## 1. Inek dosyalari (FS14 -> FS12)
- FS14 `.p3d`: `[u32 surum=1][u32 vert][u32 indis][vert*28: pos f3, normal 4xi8, tangent 4xi8, uv f2][indis*u16]...`
- FS12 `.p3d` (IndexedTriangleMesh::load, vertex stride 0x18): `[u32 vert][u32 indis][vert*24: pos f3, normal 4xi8, uv f2][indis*u16]...`
- FS14 dosyalari FS12 yukleyicisiyle OKUNAMAZ (basta ekstra surum alani + tangent). `tools/convert_p3d_fs14_to_fs12.py` cevirir
  (tangent atilir). Dogrulama: dosya boyutu birebir tuttu, indisler gecerli, normal uzunlugu ~127, UV [0,1] araliginda.
- `.p2d` dokulari (ETC1 256x256 / RGBA8 128x128) ayni baslik bicimi, degisiklik gerekmedi.
- Cevrilmis dosyalar: `assets_extra/objects/` (CI paketlemede APK `assets/objects/` icine kopyalanir).
  cowStanding, cowSleeping (+_shadow), cowStanding_diffuse, cowStanding02_diffuse, crop_manure, particle_manure, bale_grass.
  (bale.p3d / bale_shadow.p3d de cevrildi; FS12'de zaten strawbale.p3d var, gerekmezse kullanma.)
- Dikkat: cowSleeping yuksekligi 1.82 m, cowStanding 1.96 m cikti; yatan inegin bu kadar uzun olmasi COW_ANIM shader'ina
  (FS14 tepe noktalarini kaydiriyor olabilir) bagli olabilir. Ekranda gorunce kontrol et.

## 2. FS12'deki satis/depo noktalari (libapp.so decompile, Map::load + TipSite)
FS12'de 7 sabit `TipSite` var (koordinatlar x,z):
| # | tur | konum | not |
|---|-----|-------|-----|
| 0 | depo (isStorage=true) | 40.8, 182.4 | para yerine depoya yazar |
| 1 | satis | 313.8, -146.3 | fiyat carpanlari 1.5 / 1.7 / 1.5 / 1.7 |
| 2 | satis | -249.1, -353.4 | 1.5*1.2 / 1.7*1.2 / 1.5*1.25 |
| 3 | satis | -355.0, 345.0 | 1.5*1.4 / 1.7*1.4 / 1.5*1.4 |
| 4-6 | BGA (biyogaz) | -213/-224.5/-236, -43.7 | tank azami 100000, giren miktar *0.2 |
- `TipSite::tip(miktar, doluTuru, game)`: satis noktasinda para = fiyat[tur] * carpan[tur] * miktar (game+0x6a0 cift hassasli para);
  fiyat her satista duser (alt sinir %70), `update` ile zamanla toparlanir.
- Her TipSite 4 dolgu turu fiyati tasir (tur 0..3). Hangisinin "saman" oldugu henuz dogrulanmadi.
- FS12'de zaten: gulle (liquid manure) tanklari (smallManureTank_*, largeManureTank_*), `Map::getIsLiquidManurePlaceInRange`,
  balya makinesi (balemaster_*), saman balyasi (strawbale.p3d). Ahir/inek/sut KODU YOK.

## 3. Ahir yeri (BELIRLENDI)
Haritada (map01.m3d) siloler: nesne #64, #65, #66, x=43.1, z=193.0 / 204.1 / 215.3 (7.8 x 8.3 m, 19.6 m yuksek).
Tam karsilarinda (dogu, ~59 m) kapali ambar: **nesne #92**, merkez (102.3, 199.8), 39.8 x 25.0 m, 11.8 m yuksek (z'si ortadaki siloyla hizali).
Ambara giren #93 (12.6 x 2.6 m, 5.5 m yuksek) konveyor/rampa; yerde #79 (x 71-76, z 196-204) isaret padi = eski saman satis noktasi.
- Karar: #92 ve #93 gizlenir, #79 pad kalir (= ahirin balya birakma noktasi), suru (92, 200) civarina konur.
- `src/game/CowshedSite.h` koordinatlari tutar. android_entry: `hideObject(92)` ile ambar cizilmez, `CowHerd` ayni merkeze konur, kamera ambarin 30 m batisinda baslar.
- UYARI: ambarin carpisma poligonlari (fizik) kalmis olabilir -> gorunmez duvar. Haritanin poligon verisi henuz okunmuyor.
- UYARI: inek yuksekligi (kHerdY) 0; arazi yuksekligi okunmali. Ambar modeli yok (FS14 ahir modeli gelince eklenir).
- Saman satisi bu ambarda yapiliyorsa o mantik kodda gorunmuyor: FS12'nin 7 TipSite'indan hicbiri bu konumda degil (en yakin depo 40.8,182.4).
  Saman satisi baska bir mekanizmayla (ornegin balya tasima/Game) yapiliyor olabilir; Game portu gelince netlesir.

## 4. Henuz yapilmayanlar
- Game/Map/EntityManager sinifi olmadigindan oyun-ici baglanti (balya birakma, tank, para) yok.
- Inek animasyonu (COW_ANIM shader + m_keyFrames) yok; sabit durus.
- TODO(constants.txt): balya hacmi, gubre azami seviyesi, gun/gece saatleri.
- WorldView::renderCows cihazda / GL ile calistirilmadi (yalniz sozdizimi kontrol edildi).
