// stb_vorbis'i C++ tarafindan kullanilabilir sekilde bildirir (uygulama stb_vorbis_impl.c'de).
#pragma once
#define STB_VORBIS_HEADER_ONLY
extern "C" {
#include "../../third_party/stb_vorbis.c"
}
#undef STB_VORBIS_HEADER_ONLY
