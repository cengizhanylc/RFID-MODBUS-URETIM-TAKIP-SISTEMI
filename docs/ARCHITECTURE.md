# TeosSensor Mimari Notları

Bu proje için ana hedef: yeni özellik eklerken `main.cpp` büyümesin, donanım bağımlılıkları rastgele dosyalara yayılmasın ve her modülün sorumluluğu net kalsın.

## Katmanlar

- `src/main.cpp`: Arduino giriş noktasıdır; sadece uygulamaya delege eder.
- `src/app/TeosSensorApp.*`: uygulama orkestrasyonudur; servisleri başlatır, döngü sırasını yönetir ve özellikleri birbirine bağlar.
- `src/*Manager.*` ve `src/*Reader.*`: donanım, protokol veya altyapı servisleridir.
- `src/features/BootButtonReset`: BOOT/GPIO0 düğmesinden fabrika ayarına dönüş isteğini yönetir. GPIO0 önyükleme pini olduğu için NVS temizleme ve yeniden başlatma, basış algılandıktan sonra tuş bırakılınca yapılır.
- `include/Config.h`: pinler, zamanlamalar, portlar ve kalıcı anahtarlar gibi sabit ayarların tek kaynağıdır.
- `src/features/Provisioning`: NFC/web gibi kaynaklardan gelen ağ ayarı metinlerini ortak modele çevirir.

## Yeni Özellik Kuralı

Yeni bir özellik doğrudan `main.cpp` içine yazılmamalı. Özellik kendi sınıfına sahip olmalı ve `TeosSensorApp` içinde açıkça bağlanmalıdır.

Önerilen desen:

```cpp
class ExampleFeature {
public:
  bool begin();
  void loop(uint32_t currentMs);
};
```

Özellik başka bir servise ihtiyaç duyuyorsa global değişken kullanmak yerine constructor ile almalıdır:

```cpp
ExampleFeature::ExampleFeature(NvsManager &nvsManager, ModbusServer &modbusServer);
```

## Önerilen Büyüme Yapısı

Proje büyüdükçe dosyalar şu klasörlere taşınabilir:

```text
src/
  app/
    TeosSensorApp.h
    TeosSensorApp.cpp
  core/
    PeriodicTask.h
  services/
    NvsManager.*
    WifiManager.*
    ModbusServer.*
  readers/
    RfidReader.*
  web/
    WebServerManager.*
    WebTemplates.h
  features/
    AccessControl/
      AccessControlFeature.*
```

Mevcut refactor minimum risk için sadece `app` katmanını ekledi. Diğer taşıma işlemleri özellik ihtiyacı netleşince kademeli yapılmalı.

NFC tag formatı ve kullanım notları için `docs/NFC_PROVISIONING.md` dosyasına bak.

## Ek Kontrol Listesi

1. Özelliğin girdisini ve çıktısını tanımla.
2. Kalıcı veri gerekiyorsa NVS anahtarını `include/Config.h` içine ekle.
3. Donanım/protokol erişimini ayrı servis sınıfında tut.
4. Zamanlama gerekiyorsa magic number kullanma; sabiti `include/Config.h` içine koy.
5. Özelliği `src/app/TeosSensorApp.cpp` içinde başlat ve döngüye bağla.
6. Derleme başarılı olmadan yeni özelliği ikinci bir özellikle karıştırma.
