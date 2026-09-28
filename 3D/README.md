# 3D-Druckdateien – WordClock 16x16 (2024)

Sicherungskopie der Druckdateien, falls die Originalseite nicht mehr erreichbar ist.

| | |
| --- | --- |
| **Modell** | WordClock 16x16 – 2024 |
| **Urheber** | **AWSW** |
| **Original** | <https://www.printables.com/model/768062-wordclock-16x16-2024> |
| **Lizenz** | [Creative Commons BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/deed.de) (Namensnennung, nicht kommerziell) |

> Diese Dateien stehen **nicht** unter der MIT-Lizenz dieses Repositorys, sondern unter
> CC BY-NC 4.0 von AWSW. Weitergabe und Änderungen sind nur mit Namensnennung
> und **nicht kommerziell** erlaubt. Bitte nach Möglichkeit direkt über Printables herunterladen
> und dort das Original unterstützen.

## Inhalt

| Ordner / Datei | Inhalt |
| --- | --- |
| `COMMON/lid.3mf` | Deckel (hält ESP32 und drückt die LED-Matrix an) |
| `COMMON/screw1–4.3mf` | 4 optionale Abstandsschrauben für die Wandmontage |
| `DE - GERMAN/` | Frontplatte Deutsch – `v1` (Standardschrift) und `v2` (rundere Schrift) |
| `EN`, `ES`, `FR`, `IT`, `NL`, `PL`, `SWE` | Frontplatten in weiteren Sprachen (die Firmware in diesem Repo unterstützt nur Deutsch) |
| `768062-wordclock-16x16-2024-….pdf` | Export der Printables-Seite mit Anleitung, Teileliste und Bildern |

## Druckanleitung

Nach der Anleitung von AWSW auf Printables, übersetzt und zusammengefasst.

**Allgemein**
- Druckgröße 180 × 180 mm, passt auf jeden Drucker ab 200 × 200 mm (z. B. Prusa MK3S+, MINI).
- PLA in **Schwarz** und **Weiß**.
- Keine Stützen nötig, alle Teile flach auf die Druckplatte legen.
- Profil 0,3 mm „Draft“ (0,2 mm geht auch).
- Haftung: Druckplatte vorher mit Spüli und danach mit Isopropanol reinigen. Bei Problemen einen
  5-mm-Brim für Frontplatte und Deckel ergänzen.
- Frontplatte: in PrusaSlicer „Avoid crossing perimeters“ aktivieren.
- Abstandsschrauben (`screw1–4`) mit 30–50 % Infill (15 % geht auch).

**Frontplatte mit Diffusor – zwei Farbwechsel**

Die weiße Schicht in der Mitte der Frontplatte ist der Diffusor. Dafür wird zweimal die Farbe gewechselt:

| Abschnitt | Farbe | bei 0,2-mm-Layer | bei 0,3-mm-Layer |
| --- | --- | --- | --- |
| 1 | Schwarz | 0 – 1,2 mm | 0 – 1,1 mm |
| 2 | Weiß | 1,2 – 2,4 mm | 1,2 – 2,6 mm |
| 3 | Schwarz | ab 2,4 mm bis Ende | ab 2,6 mm bis Ende |

In den 3MF-Dateien sind die Farbwechsel für 0,3 mm bereits eingestellt.

**Ablösen**
- Bei strukturierter Druckplatte die Frontplatte **komplett abkühlen lassen** (ca. 45–60 Minuten),
  bis sie sich von selbst löst. Nicht abreißen, sonst reißen die Buchstaben heraus.

## Teile

| Anzahl | Teil |
| --- | --- |
| 1 | 16x16-WS2812B-LED-Matrix (das Gehäuse passt genau zu dieser Matrix) |
| 1 | NodeMCU ESP32 mit USB-C (das Gehäuse passt genau zu diesem Board) |
| 1 | USB-Netzteil 5 V / 3 A – **kein** 5 V / 1–2 A |
| 1 | USB-A-auf-USB-C-Kabel (5 V, 3 A) – USB-C-Netzteile direkt funktionieren nicht |
| 1 | USB-C-Einbaubuchse (5 V, 3 A) |
| 3 | Wago 221-413 Klemmen |
| 3 | Jumperkabel Male-Female, 20 cm |
| 3 | Schrauben M3×16 |
| 4 | Schrauben M3×8 |
| 4 | kleine Kabelbinder |

Werkzeug: Abisolierzange, Innensechskant M3, Seitenschneider, optional Lötkolben.

## Zusammenbau

**Verkabelung**

| Wago-Klemme | USB-C-Buchse | ESP32 | LED-Matrix | Farbe |
| --- | --- | --- | --- | --- |
| 1 | 5V | VIN | 5V | Rot |
| 2 | GND | GND | GND | Schwarz / Weiß |
| 3 | – | **D32** | DIN | Grün |

Die LED-Matrix wird direkt über die Buchse versorgt, nicht über den ESP32. Das schont das Board
bei hoher Helligkeit.

**Schritte**

1. An der LED-Matrix die mittleren und unteren Kabelpaare abschneiden (Kurzschlussgefahr).
   Nur die Kabel am **DIN**-Anschluss bleiben dran.
2. LED-Matrix in der richtigen Ausrichtung in die Frontplatte legen (Bilder im PDF).
3. Die zwei Kabel der USB-C-Buchse **11 mm** abisolieren (Markierung „11“ im Gehäuse hilft)
   und die Buchse in den Deckel drücken, bis sie einrastet.
4. Die 3 Wago-Klemmen öffnen und in ihre Halter im Deckel setzen (Nummern 1, 2, 3 unter den Klemmen).
5. USB-C-Buchse an Klemme 1 (5V) und 2 (GND) anschließen.
6. Jumperkabel mit dem Stecker-Ende in die Klemmen: Rot → 1, Schwarz → 2, Grün → 3.
7. Die Buchsen-Enden auf den ESP32 stecken: Rot → **VIN**, Schwarz → **GND**, Grün → **D32**.
8. ESP32 mit den Kabeln in die Halterung im Deckel drücken, Kabel durch die Schlitze führen
   und den ESP32 mit den 4 Schrauben **M3×8** befestigen.
9. Den mitgelieferten Stecker der LED-Matrix ebenfalls 11 mm abisolieren und anschließen:
   Rot → 1, Weiß → 2, Grün → 3. Alle Klemmen schließen.
10. USB-C-Stecker des Kabels durch das Loch im Deckel führen und in die Buchse stecken.
11. Alles nochmal prüfen, Kabel mit Kabelbindern sichern – vor allem das Stromkabel.
12. **Firmware flashen**, falls noch nicht geschehen (siehe [Haupt-README](../README.md#firmware-flashen)).
13. LED-Matrix an ihren Stecker im Deckel anschließen.
14. Deckel vorsichtig auf die Frontplatte schieben. Er drückt die Matrix an, Kleber ist nicht nötig.
15. Deckel mit den 3 Schrauben **M3×16** festschrauben.
16. Netzteil (5 V / 3 A) anschließen und WLAN einrichten
    (siehe [Haupt-README](../README.md#wlan-einrichten)).
