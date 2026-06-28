# NFC ile AP Modunda Ağ Ayarı

PN532, cihaz AP modundayken NFC tag içindeki metni okur. Metin geçerliyse web panelinde değiştirilebilen ağ alanları NVS'e kaydedilir ve cihaz yeniden başlamadan STA moduna geçmeye çalışır.

## Önerilen TXT Formatı

NFC yazma uygulamasında tag tipini `Text` seçip şu içeriği yaz:

```text
ssid=OfisWifi
pass=12345678
ip=192.168.1.100
gateway=192.168.1.1
subnet=255.255.255.0
```

Desteklenen alan adları:

- `ssid`, `wifi_ssid`, `s`
- `pass`, `password`, `wifi_pass`, `sifre`, `p`
- `ip`, `ip_address`, `ip_adresi`, `i`
- `gateway`, `gateway_ip`, `gw`, `g`
- `subnet`, `subnet_mask`, `mask`, `netmask`, `n`

## JSON Formatı

İstersen kısa veya uzun JSON da yazabilirsin:

```json
{"ssid":"OfisWifi","pass":"12345678","ip":"192.168.1.100","gateway":"192.168.1.1","subnet":"255.255.255.0"}
```

Kısa alanlı eski format da desteklenir:

```json
{"s":"OfisWifi","p":"12345678","i":"192.168.1.100","g":"192.168.1.1","n":"255.255.255.0"}
```

## Davranış

- NFC ayarı sadece AP modunda işlenir.
- `ssid` zorunludur.
- IP, gateway veya subnet boş bırakılırsa varsayılan değer kullanılır.
- IP değerleri geçersizse kayıt yapılmaz.
- Şifre boşsa ve aynı SSID daha önce kayıtlıysa eski şifre korunur; değilse açık ağ olarak kaydedilir.
- STA bağlantısı başarılıysa AP kapanır, cihaz verilen statik IP ile ağa katılır ve Modbus başlatılır.
- STA bağlantısı başarısızsa cihaz AP provisioning moduna geri döner.
