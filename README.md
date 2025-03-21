# 🛡️ USB Tracker Service (Windows)   

Bu proje, **Windows işletim sistemi üzerinde çalışan bir servis** olarak geliştirilmiş olup, **USB cihazlarının takılma ve çıkarılma olaylarını tespit ederek sistem günlüklerine kaydeder**. Servis arka planda çalışır ve kullanıcı etkileşimi olmadan sistem güvenliğini artırmak için kullanılabilir.

---

## 📌 Özellikler  
- ✅ **Servis olarak çalışır** – Windows servis olarak başlatılır/durdurulur.  
- ✅ **USB Cihaz Tespiti** – Takılan ve çıkarılan USB cihazlarını anlık olarak algılar.  
- ✅ **Loglama** – Cihaz bilgilerini (VID, PID, üretici, seri numarası, IP ve bilgisayar adı) `C:\USBTrackerLog.txt` dosyasına kaydeder.  
- ✅ **Gizli Pencere (Hidden Window)** – Olayları yakalamak için arka planda gizli pencere oluşturur.  

---

## 🛠 Kullanılan Teknolojiler  
- **C++ (Win32 API)** – Servis yapısı ve sistem olaylarını yönetmek için  
- **Windows SetupAPI & Device Notification API** – USB cihaz bilgilerini almak için  
- **WS2_32** – IP adresi tespiti için Winsock kullanımı  
- **Multithreading & Event Handling** – Servis thread'i ve mesaj döngüsü  
- **Log Sistemi** – Basit dosya yazma sistemiyle log tutma  

---

## 🚀 Nasıl Çalışır?  
1. Servis başlatıldığında `ServiceWorkerThread` fonksiyonu bir gizli pencere oluşturur.  
2. Bu pencere, `WM_DEVICECHANGE` olaylarını dinler.  
3. Yeni bir USB cihazı takıldığında ya da çıkarıldığında, `HiddenWndProc` bu olayları işleyip `WriteLog()` fonksiyonu ile kayıt altına alır.  
4. Kaydedilen bilgiler:
   - PC adı  
   - IP adresi  
   - Cihaz tanımı  
   - Üretici  
   - VID, PID  
   - Seri numarası

---

## 🧾 Örnek Log Çıktısı

Servis çalıştığında `C:\USBTrackerLog.txt`

[+] USB Connected: PC: LAPTOP-12345 | IP: 123.456.7.89 | Device: USB Mass Storage Device | Manufacturer: SanDisk | VID: 0781 | PID: 5581 | Serial: 1234567890ABCDEF

[-] USB Disconnected: PC: LAPTOP-12345 | IP: 123.456.7.89 | Device: USB Mass Storage Device | Manufacturer: SanDisk | VID: 0781 | PID: 5581 | Serial: 1234567890ABCDEF

---

## 📦 Kurulum ve Yönetim  

> ⚠ **Not:** Servisi yüklemek, başlatmak ve kaldırmak için **yönetici yetkileri** gereklidir.

### 🔹 Servisi Kurma ve Başlatma  
Aşağıdaki komutları **Komut İstemi (CMD) veya PowerShell'i** **yönetici olarak çalıştırarak** uygulayın: 

sc create USBTrackerService binPath= "C:\Path\To\USBTrackerService.exe"
sc start USBTrackerService
sc stop USBTrackerService
sc delete USBTrackerService

---

## 🔐 Gelecek Geliştirmeler 

📌 **TCP/IP Tabanlı Veri Aktarımı**  
İstemci servisleri tarafından algılanan tüm USB olaylarının, merkezi yönetim bilgisayarına **gerçek zamanlı olarak TCP/IP protokolü üzerinden** gönderilmesi sağlanacak.

📌 **PostgreSQL Veritabanı Entegrasyonu**  
Tüm USB olayları **PostgreSQL veritabanına kaydedilecek.** Loglarda aşağıdaki bilgiler yer alacak:
- VID / PID bilgileri  
- Seri numarası  
- Cihaz açıklaması ve üretici bilgisi  
- Bilgisayar adı ve IP adresi  
- Bağlantı zamanı (timestamp)

📌 **USB Kimliği Tanımlama ve Takip**  
Her USB cihazının **benzersiz kimliği** (VID, PID, Serial) kayıt altına alınarak:  
- Aynı cihazın farklı bilgisayarlarda kullanımı izlenebilecek  
- Belirli cihazlara özel izin politikaları tanımlanabilecek

📌 **Servis Durum Arayüzü (Service Monitor GUI)**  
Ana yönetim uygulaması üzerinden:  
- İstemci servislerin çalışma durumu  
- Anlık bağlantı sayısı  
- Log trafiği ve ağ durumu  
gibi bilgiler **görsel arayüz üzerinden** izlenebilir hale getirilecek.

📌 **Yönetici Paneli ile Erişim Kontrolü**  
Merkezi bir **admin paneli** ile:  
- Hangi cihazlara erişim izni verileceği  
- Hangi cihazların engelleneceği  
- Hangi kullanıcıların / bilgisayarların hangi cihazları kullanabileceği  
kolayca yönetilebilecek.

📌 **USB Politikaları Tanımlama**  
- Cihaz bazlı, kullanıcı bazlı veya bilgisayar bazlı **izin/engelleme kuralları** tanımlanabilecek  
- Örnek: “Sadece bu seri numaralı USB’ye izin ver” veya “Tüm bilinmeyen cihazları engelle”

---

## 📃 **Lisans**  
Bu proje **MIT Lisansı** ile lisanslanmıştır. Serbestçe kullanılabilir ancak ticari amaçlarla kullanırken referans verilmesi önerilir.  

------------------------------------------------------------------------

# 🛡️ USB Tracker Service (Windows)  

This project is developed as a **Windows service** that detects USB device connection and disconnection events and logs them into the system. The service runs in the background and enhances security without user interaction.  

---

## 📌 Features  
- ✅ **Runs as a Windows Service** – Can be started/stopped like a standard Windows service.  
- ✅ **USB Device Detection** – Monitors USB devices in real-time.  
- ✅ **Logging** – Saves device details (VID, PID, manufacturer, serial number, IP, and PC name) to `C:\USBTrackerLog.txt`.  
- ✅ **Hidden Window (Background Event Listener)** – Uses a hidden window to capture device change events.  

---

## 🛠 Technologies Used  
- **C++ (Win32 API)** – For service management and system events handling.  
- **Windows SetupAPI & Device Notification API** – To retrieve USB device details.  
- **WS2_32 (Winsock)** – To fetch IP address details.  
- **Multithreading & Event Handling** – To ensure efficient service operation.  
- **Logging System** – Writes logs to a file for later analysis.  

---

## 🚀 How It Works  
1. When the service starts, `ServiceWorkerThread` creates a hidden window.  
2. This window listens for `WM_DEVICECHANGE` events.  
3. When a new USB device is connected or disconnected, `HiddenWndProc` processes the event and logs it using `WriteLog()`.  
4. The logged information includes:  
   - PC Name  
   - IP Address  
   - Device Description  
   - Manufacturer  
   - VID & PID  
   - Serial Number  

---

## 🧾 Example Log Output  

When the service runs, `C:\USBTrackerLog.txt` records logs as follows:  

[+] USB Connected: PC: LAPTOP-12345 | IP: 192.168.1.25 | Device: USB Mass Storage Device | Manufacturer: SanDisk | VID: 0781 | PID: 5581 | Serial: 1234567890ABCDEF
[-] USB Disconnected: PC: LAPTOP-12345 | IP: 192.168.1.25 | Device: USB Mass Storage Device | Manufacturer: SanDisk | VID: 0781 | PID: 5581 | Serial: 1234567890ABCDEF

---

## 🔐 Planned Features

**📡 TCP/IP-Based Data Transmission**  
Client-side services will send all detected USB events to the central management system **in real-time over TCP/IP.**

**🗄 PostgreSQL Database Integration**  
All USB activity will be stored in a **PostgreSQL database**, including:
- VID / PID  
- Serial number  
- Device description & manufacturer  
- Host computer name & IP address  
- Timestamp

**🔍 USB Identification and Tracking**  
Each USB device will be uniquely identified using VID, PID, and Serial Number:
- Track the same device across multiple computers  
- Apply custom access rules to specific devices

**🖥 Service Monitor GUI**  
The main control application will display:
- Client service status  
- Active connection count  
- Log traffic and network health  
All through a **graphical user interface**

**🔐 Access Control via Admin Panel**  
Through a centralized admin panel, you will be able to:
- Allow or block specific USB devices  
- Manage access permissions per user or computer  
- Enforce security policies easily

**⚙ USB Policy Rules**  
Create allow/block rules based on:
- Device ID  
- User identity  
- Host computer  
Examples:  
- "Allow only USB with this serial number"  
- "Block all unknown USB devices"

---

## 📃 License

This project is licensed under the **MIT License**.  
You can use it freely. Attribution is appreciated for commercial use.
