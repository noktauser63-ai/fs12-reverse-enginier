#include "engine.h"

namespace fs12 {
namespace audio {
#ifndef __ANDROID__
Output* createDefaultOutput() { return nullptr; }
#endif
}  // namespace audio
}  // namespace fs12
