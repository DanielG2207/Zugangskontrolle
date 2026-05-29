#include <SPI.h> // Bibliothek für Kommunikation vom RFID-Scanner
#include <MFRC522.h> // Bibliothek für die Steuerung vom RFID-Scanner
#include <LiquidCrystal.h> // Bibliothek für LCD-Display
#include <Keypad.h>  // Bibliothek für Tastenfeld 

// Hardware-Pins Definition 
#define SS_PIN 53
#define RST_PIN 49
MFRC522 rfid(SS_PIN, RST_PIN);  // RFID-Leser-Objekt erstellen (wird in setup() initialisiert)

// LCD Display mit 16x2 Zeichen
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#define LED_BLAU  24 
#define LED_GRUEN 22 
#define LED_ROT   23 
#define SUMMER    25  
#define SCHLOSS   26  


// Authentifizierung - PIN und Karten
String korrekterPIN = "1234"; // PIN Code festlegen

// Tastenfeld Konfiguration - 4x4 Matrix
const byte ZEILEN = 4;
const byte SPALTEN = 4;
// Layout des Tastenfelds
char tasten[ZEILEN][SPALTEN] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte zeilenPins[ZEILEN] = {45, 43, 41, 39};  // Pins der Zeilen am Arduino Mega 
byte spaltenPins[SPALTEN] = {37, 35, 33, 31}; // Pins der Spalten am Arduino Mega 
// Tastenfeld wird mit Zeilen- und Spaltenpins initialisiert
Keypad tastenfeld = Keypad(makeKeymap(tasten), zeilenPins, spaltenPins, ZEILEN, SPALTEN);


// Erlaubte RFID-Karten - Whitelist

byte erlaubteUID[][4] = {
  {0xA0, 0x84, 0xB5, 0x56}  // Karte 1 - UID in hexadezimal
};
const int anzahlKarten = 1;  // Anzahl der erlaubten Karten

// Speichert die vom Benutzer eingegebene PIN
String eingegebenerPIN = "";

// Zugang gewährt
// Diese Funktion wird verwendet, wenn die Authentifizierung erfolgreich ist
void zugang_gewaehrt() {
  // Grüne LED anschalten 
  digitalWrite(LED_GRUEN, HIGH);
  // Elektromagnetisches Schloss entsperren
  digitalWrite(SCHLOSS, HIGH);
  // Piepston abspielen (500 Hz)
  tone(SUMMER, 500);
  // Display-Meldung anzeigen
  lcd.clear();
  lcd.print("Zugang");
  lcd.setCursor(0, 1);
  lcd.print("gewaehrt!");
  // 2 Sekunden warten 
  delay(2000);
  // Piepston beenden
  noTone(SUMMER);
  // LED und Schloss ausschalten
  digitalWrite(LED_GRUEN, LOW);
  digitalWrite(SCHLOSS, LOW);
  // Zurück zur Standby-Meldung
  lcd.clear();
  lcd.print("Bitte Karte");
  lcd.setCursor(0, 1);
  lcd.print("vorhalten...");
}

// Zugang verweigert
// Diese Funktion wird verwendet, wenn die Authentifizierung fehlschlägt
void zugang_verweigert() {
  // Rote LED anschalten 
  digitalWrite(LED_ROT, HIGH);
  // Piepston abspielen (300 Hz)
  tone(SUMMER, 300);
  // Display-Meldung anzeigen
  lcd.clear();
  lcd.print("Zugang");
  lcd.setCursor(0, 1);
  lcd.print("verweigert!");
  // 2 Sekunden warten
  delay(2000);
  // Piepston beenden
  noTone(SUMMER);
  // LED ausschalten
  digitalWrite(LED_ROT, LOW);
  // Zurück zur Standby-Meldung
  lcd.clear();
  lcd.print("Bitte Karte");
  lcd.setCursor(0, 1);
  lcd.print("vorhalten...");
}

// Initialisierung beim Start
void setup() {
  // Serielle Verbindung für Debugging (9600 Baud)
  Serial.begin(9600);
  // SPI-Kommunikation für RFID-Leser starten
  SPI.begin();
  // RFID-Leser initialisieren
  rfid.PCD_Init();

  // Alle Ausgabe-Pins als OUTPUT definieren
  pinMode(LED_BLAU,  OUTPUT);
  pinMode(LED_GRUEN, OUTPUT);
  pinMode(LED_ROT,   OUTPUT);
  pinMode(SUMMER,    OUTPUT);
  pinMode(SCHLOSS,   OUTPUT);

  // Standby Zustände: Blaue LED an, Schloss zu
  digitalWrite(LED_BLAU, HIGH);
  digitalWrite(SCHLOSS, LOW);

  // LCD initialisieren (16 Spalten, 2 Zeilen) und Standby-Meldung
  lcd.begin(16, 2);
  lcd.print("Bitte Karte");
  lcd.setCursor(0, 1);
  lcd.print("vorhalten...");
}

// Hauptprogramm - Läuft wiederholt
void loop() {
  // RFID-Karten prüfen
  // Prüft, ob eine neue Karte erkannt wurde
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    bool zugang = false;
    
    // Vergleicht die Karten-UID mit der Whitelist
    for (int i = 0; i < anzahlKarten; i++) {
      // Vergleicht alle 4 Bytes der UID
      if (memcmp(rfid.uid.uidByte, erlaubteUID[i], 4) == 0) {
        zugang = true;  // Karte ist autorisiert
        break;
      }
    }
    
    // Entsprechende Funktion aufrufen
    if (zugang) zugang_gewaehrt();
    else zugang_verweigert();
    
    // Karte als verarbeitet kennzeichnen
    rfid.PICC_HaltA();
  }

  // Tastenfeld prüfen
  // Prüft, ob eine Taste auf dem Tastenfeld gedrückt wurde
  char taste = tastenfeld.getKey();
  if (taste) {
    if (taste == '#') {
      // # = PIN-Eingabe bestätigen/abschließen
      if (eingegebenerPIN == korrekterPIN) {
        eingegebenerPIN = "";  // PIN zurücksetzen
        zugang_gewaehrt();  // Zugang gewähren
      } else {
        eingegebenerPIN = "";  // Falsche PIN zurücksetzen
        zugang_verweigert();  // Zugang verweigern
      }
    } else if (taste == '*') {
      // * = Eingabe löschen
      eingegebenerPIN = "";
      lcd.clear();
      lcd.print("Eingabe");
      lcd.setCursor(0, 1);
      lcd.print("geloescht");
      delay(1000);
      // Zurück zur Standby-Meldung
      lcd.clear();
      lcd.print("Bitte Karte");
      lcd.setCursor(0, 1);
      lcd.print("vorhalten...");
    } else {
      eingegebenerPIN += taste;
      lcd.clear();
      lcd.print("PIN eingeben:");
      lcd.setCursor(0, 1);
      // Sterne anzeigen statt echtem PIN 
      for (int i = 0; i < eingegebenerPIN.length(); i++) {
        lcd.print("*");
      }
    }
  }
}
