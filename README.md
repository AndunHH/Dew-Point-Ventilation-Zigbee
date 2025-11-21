# Dew-Point Ventilation -- Zigbee (Extended Fork by rneppi)

Dieses Projekt ist eine erweiterte Version des ursprünglichen\
[Dew-Point-Ventilation-Zigbee](https://github.com/AndunHH/Dew-Point-Ventilation-Zigbee)
von *AndunHH*.\
Es ergänzt mehrere nützliche Funktionen und Verbesserungen, ohne den
Kern des Systems zu verändern.

💡 *Alle neuen Funktionen sind vollständig kompatibel zum
Originalprojekt und können via Pull Request übernommen werden.*

## Inhaltsverzeichnis

1.  Überblick
2.  Neue Funktionen in diesem Fork
3.  Hardware
4.  Installation & Flashen
5.  Dateisystem & Logging
6.  Changelog (Fork)
7.  Lizenz

## Überblick

Der Dew-Point-Ventilator misst Innen- und Außentemperatur sowie
Luftfeuchtigkeit und steuert einen Lüfter so, dass möglichst
energieeffizient und taupunktoptimiert gelüftet wird.

Ein PCF8563-RTC-Modul sorgt für eine präzise Echtzeituhr, eine SD-Karte
speichert Messwerte, und über Zigbee kann der Lüfter geschaltet werden.

Dieses Fork fügt Verbesserungen hinzu, die im Langzeitbetrieb und bei
der täglichen Nutzung besonders hilfreich sind.

## Neue Funktionen in diesem Fork

### 1. Display-Sleep-Mode

OLED-Displays nutzen sich pixelweise ab. Nach rund einem halben Jahr
Dauerbetrieb waren deutliche Helligkeitsunterschiede sichtbar:\
häufig genutzte Pixel wurden merklich dunkler.

#### Warum diese Funktion wichtig ist

-   Verhindert sichtbaren Burn-In\
-   Verlängert drastisch die Lebensdauer des OLED\
-   Reduziert Stromverbrauch\
-   System bleibt trotzdem vollständig funktionsfähig

#### Wie der Modus funktioniert

-   Nach **10 Minuten ohne Benutzereingabe** schaltet das Display
    automatisch ab.
-   Alle Kernfunktionen laufen weiter wie gewohnt.
-   Die grüne LED dient als Alive-Indikator.

#### Wie man das Display wieder einschaltet

-   Knopf drücken → Display sofort aktiv.

### 2. Automatische Sommer-/Winterzeit (DST)

Die PCF8563-RTC ist erstaunlich genau --- eine externe NTP-Zeitquelle
ist nicht erforderlich.

#### Was die DST-Logik macht

-   Automatische Umschaltung:
    -   letzte Märzwoche (Sommerzeit)
    -   letzte Oktoberwoche (Winterzeit)
-   Timestamps und Dateinamen berücksichtigen die Zeitzone automatisch.

### 3. Manuelle Zeiteingabe über serielle Schnittstelle

Ermöglicht das Setzen der Zeit ohne Reflash des Geräts.

#### Vorteile

-   Test der DST-Logik\
-   Stellen der Uhr nach Batteriewechsel\
-   Plattformunabhängig (CoolTerm, PuTTY, screen, VS Code, ...)

## Hardware

(Unverändert aus dem Originalprojekt.)

## Installation & Flashen

-   Projekt mit PlatformIO öffnen\
-   Flashen über USB\
-   Display/Sensoren werden automatisch erkannt

## Dateisystem & Logging

-   CSV-Logging\
-   Monatsdateien\
-   DST wirkt sich auf Dateinamen aus

## Changelog (Fork)

### v3.2 -- Erweiterungen

-   Display-Sleep-Mode\
-   Automatische DST-Umschaltung\
-   Serielle Zeiteingabe\
-   Verbesserte `.gitignore`, Entfernen von `.DS_Store`\
-   Refactoring von `SerialTimeHelper` und `rtchelper`

## Lizenz

Apache License 2.0
