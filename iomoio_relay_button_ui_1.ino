#include <ESP8266FtpServer.h>

// Невеличкий приклад для роботи з ESP8266. Джерело: https://youtu.be/TPmPBY8XLJE

#include <ESP8266WiFi.h>            // Бібліотека для створення Wi-Fi підключення (клієнт або точка доступу)
#include <ESP8266WebServer.h>       // Бібліотека для управління пристроєм через HTTP (наприклад з браузера)
#include <FS.h>                     // Бібліотека для роботи з файловою системою
#include <ESP8266FtpServer.h>       // Бібліотека для роботи з SPIFFS через FTP

const byte relay = 2;               // Пін для підключення сигнального контакту реле
const char *ssid = "MyESP";         // Назва створюваної точки доступу

ESP8266WebServer HTTP(80);          // Оголошуємо об'єкт та порт сервера для роботи з HTTP
FtpServer ftpSrv;                   // Оголошуємо об'єкт для роботи з модулем через FTP (для налагодження HTML)

void setup() {
  pinMode(relay, OUTPUT);             // Визначаємо пін реле як вихідний
  digitalWrite(relay, HIGH);          // Встановлюємо високу напругу на виході піна (3,3В)
  Serial.begin(9600);                 // Ініціалізуємо вивід даних на послідовний порт зі швидкістю 9600 бод

  WiFi.softAP(ssid);                  // Створюємо точку доступу

  SPIFFS.begin();                     // Ініціалізуємо роботу з файловою системою
  HTTP.begin();                       // Ініціалізуємо Web-сервер
  ftpSrv.begin("relay", "relay");     // Піднімаємо FTP-сервер для зручності налагодження роботи HTML (логін: relay, пароль: relay)

  Serial.print("\nMy IP to connect via Web-Browser or FTP: "); // Виводимо на монітор послідовного порту повідомлення про те, що зараз будемо виводити локальну IP-адресу//
  Serial.println(WiFi.softAPIP());                             // Виводимо локальну IP-адресу ESP8266 в консоль
  Serial.println("\n");                                        // Переходимо на наступну строку 

  // Обробляємо HTTP-запити
  HTTP.on("/relay_switch", [](){                      // При HTTP запиті типу http://192.168.4.1/relay_switch
   HTTP.send(200, "text/plain", relay_switch());      // Відправляємо клієнту код успішної обробки запиту, повідомляємо, що формат відповіді текстовий та повертаємо результат виконання функції relay_switch
  });
  HTTP.on("/relay_status", [](){                      // При HTTP запиті типу http://192.168.4.1/relay_status
    HTTP.send(200, "text/plain", relay_status());     // Відправляємо клієнту код успішної обробки запиту, повідомляємо, що формат відповіді текстовий та повертаємо результат виконання функції relay_status
  });
  HTTP.onNotFound([](){                               // Описуємо дії при події "Не знайдено"
  if(!handleFileRead(HTTP.uri()))                     // Якщо функція handleFileRead (описана нижче) повертає значення false відповідно до пошуку файлу в файловій системі
    HTTP.send(404, "text/plain", "Not Found");        // Повертаємо текстове повідомлення "Файл не знайдено" з кодом 404 (не знайдено)
  });
}

void loop() {
  HTTP.handleClient();                  // Обробник HTTP-подій (отримує запити HTTP до пристрою та обробляє їх відповідно до вище описаного алгоритму)
  ftpSrv.handleFTP();                    // Обробник з'єднань FTP
}

String relay_switch() {                 // Функція перемикання реле
  byte state;
  if (digitalRead(relay))               // Якщо на виводі реле високий рівень
    state = 0;                          // то запам'ятовуємо, що його потрібно змінити на низький
  else                                  // інакше
    state = 1;                          // запам'ятовуємо, що потрібно змінити його на високий
  digitalWrite(relay, state);           // змінюємо значення на виводі підключення реле
  return String(!state);                // повертаємо результат, перетворивши число в рядок
}

String relay_status() {                 // Функція для визначення поточного стану реле
  byte state;
  if (!digitalRead(relay))              // Якщо на виводі реле високий рівень
    state = 1;                          // то запам'ятовуємо його як одиницю
  else                                  // інакше
    state = 0;                          // запам'ятовуємо його як нуль
  return String(state);                 // повертаємо результат, перетворивши число в рядок
}

bool handleFileRead(String path){                     // Функція роботи з файловою системою
  if(path.endsWith("/")) path += "index.html";        // Якщо пристрій викликається за кореневою адресою, то має викликатись файл index.html (додаємо його в кінець адреси)
    String contentType = getContentType(path);        // За допомогою функції getContentType (описаної нижче) визначаємо тип файлу (в адресі звернення), який потрібно повернути при його виклику
  if(SPIFFS.exists(path)){                            // Якщо в файловій системі існує файл за адресою звернення
    File file = SPIFFS.open(path, "r");               // Відкриваємо файл для читання
    size_t sent = HTTP.streamFile(file, contentType); // Виводимо вміст файлу через HTTP, вказуючи заголовок типу вмісту contentType
    file.close();                                     // Закриваємо файл
    return true;                                      // Завершуємо виконання функції, повертаючи результатом true (істина)
  }
  return false;                                       // Завершуємо виконання функції, повертаючи результатом false (якщо попередній умова не виконана)
}

String getContentType(String filename) {              // Функція getContentType повертає заголовок типу контенту в залежності від розширення файлу
  if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else {
    return "text/plain";
  }
}

