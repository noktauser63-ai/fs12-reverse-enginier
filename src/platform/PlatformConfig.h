// Lisans / satin alma / bulut yedegi: bu yapida kaldirildi (kullanici istegi).
#pragma once
#include <string>
namespace fs12 {

// Orijinal IAP (Google Play Billing) arayuzunun yerine gecen bos cihaz.
// Oyun kodu bunu "urun yok / satin alma kapali" olarak gorur.
class PurchaseDevice {
public:
    virtual ~PurchaseDevice() {}
    virtual bool available() const { return false; }
    virtual bool canPurchase(int) const { return false; }
    virtual bool canRestore() const { return false; }
    virtual int  getState(int) const { return 0; }
    virtual std::string getName(int) const { return std::string(); }
    virtual std::string getDescription(int) const { return std::string(); }
    virtual std::string getPrice(int) const { return std::string(); }
    virtual void requestPurchase(int) {}
    virtual void requestRestore() {}
};
using NullPurchaseDevice = PurchaseDevice;

} // namespace fs12
