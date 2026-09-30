#ifndef BOOT_BUTTON_RESET_H
#define BOOT_BUTTON_RESET_H

#include <Arduino.h>

// BOOT (GPIO0) düğmesine çalışma anında basılıp bırakıldığında fabrika ayarına
// dönüş isteği üretir. GPIO0 bir önyükleme pini olduğundan işlem tuş
// bırakıldıktan sonra uygulama tarafından yapılır; böylece cihaz programlama
// modunda kalmaz.
class BootButtonReset {
public:
  void begin();
  void loop(uint32_t currentMs);
  bool consumeFactoryResetRequest();

private:
  bool rawPressed_ = false;
  bool stablePressed_ = false;
  bool armed_ = false;
  bool factoryResetPending_ = false;
  bool factoryResetRequest_ = false;
  uint32_t lastRawChangeMs_ = 0;
};

#endif // BOOT_BUTTON_RESET_H
