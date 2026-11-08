#include "ModBridge.h"
namespace fs12 {
ModBridge& modBridge() { static ModBridge b; return b; }
}
