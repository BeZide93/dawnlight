# Hyrule Survivors: zeitgesteuerte Elites und Bosse

Codeprüfung vom 19.10.2026 für PR #140. Grundlage: Survivors-Roster und
KI-Adapter sowie die nativen Dusklight-Aktoren, SDK-Revision
`e9b120544cb75e81b5aa36777f1688fa61f2d9e8`. Keine Messung im laufenden Spiel.

Die nativen Tabellen bleiben als Ausgangsbasis erhalten. Die zusätzlichen
Angriffsfolgen, Projektile, Spawnpause und 10-s-Rücksetzung aus PR #148 stehen im
[aktuellen Boss-Taktik-Referenzblatt](hyrule-survivors-boss-tactics.md). Diese
Taktikänderungen erhöhen den Schaden nicht. Ein nachfolgender Balance-Patch
verdoppelt die HP aller vier zeitgesteuerten Bosse einschließlich Finalboss und
Endless-Wiederholungen; stationäre Wächter behalten ihre bisherigen HP.


Aktualisierung für PR #154 (23.10.2026): Hyrule Field verwendet jetzt
Riesen-Bokoblin → Bullbo mit zwei Bulblins → Aeralfos → King Bulblin. Die
historischen nativen Ausgangstabellen unten beschreiben noch die ersetzten
Gegner. Die HP-Budgets der Ersatzslots bleiben bestehen. Bullbo, beide Reiter
(inklusive Pfeile) und der Elite-Aeralfos verursachen mit allen Angriffen zwei
Herzen Basisschaden vor Schwierigkeit/Endless und Schadensreduktion. Der Bullbo hat 150% große Zielradien; sein Bogenschütze feuert immer
dreifach mit halbem Bereitschaftstimer. Seine Lauf-KI prüft die Bewegung vorab
gegen den Lichtkreis (inklusive Endless-Tempo), wendet vor der Wand und kehrt
in die innere Kreishälfte zurück, bevor sie Link wieder verfolgt. Der Aeralfos wechselt alle 10 aktiven
Sekunden zwischen Boden und Luft, fliegt im halbierten Höhenband 250–400 über
Boden und hat auf ein Viertel verkürzte native Bereitschaftstimer. Die niedrigere Obergrenze beendet
auch den Rückflug nach einem Sturzangriff. Bereite Luftangriffe warten nicht mehr
auf den nativen Partnerstatus. Der Sturzflug erfasst Links Position beim Beginn
des sichtbaren Ausholens und fliegt danach mit 60 Einheiten pro Update auf einer
festen Bahn etwa 450 Einheiten darüber hinaus. Er verfolgt Link nicht weiter
und bremst nicht in seiner Nähe. Auf der erfassten Körperhöhe wird der Flug
waagerecht; Wand, Bodenkontakt, Lichtkreis oder drei Sekunden Sicherheitslimit
beenden ihn früher. Die anschließende Erholung verursacht keinen Schaden.
Im Bodenkampf wird alle drei aktiven Sekunden ein
kollisionsgeprüfter Gap Closer fällig; laufende Schläge werden erst beendet.
Nach dem Ansturm beginnt sofort der native Nahkampfangriff. Blockierte Wege
brechen den Ansturm ab. Gezielte Sword Waves von King Bulblin berücksichtigen
auch Links Höhe beim Abschuss; sie verfolgen ihn danach nicht weiter.

## Zeiten und Schadenseinheiten

Die vier Slots erscheinen bei 25 %, 50 %, 75 % und 100 % der Run-Dauer:

| Run | Slot 1 | Slot 2 | Slot 3 | Finalboss |
|---|---|---|---|---|
| 20 Minuten | 5:00 | 10:00 | 15:00 | 20:00 |
| 10 Minuten | 2:30 | 5:00 | 7:30 | 10:00 |
| 5 Minuten | 1:15 | 2:30 | 3:45 | 5:00 |

Ein fehlender sicherer Spawnplatz kann das tatsächliche Erscheinen verzögern.
Die stationären Gebäudewächter gehören nicht zu diesen vier Slots.

Ein Logikframe (F) entspricht 1/30 Sekunde. Zufallsspannen sind ganzzahlige
Timerwerte; beispielsweise `30 + rndF(30)` bedeutet 30–59 F. Angegebene
Wartezeiten sind **keine garantierten Abstände zwischen Treffern**:
Animationen, Anflug, Ausrichtung, Reichweite, Blocken und Stagger kommen hinzu.
Manche Timer laufen bereits während des Angriffs ab. Die kürzeren Debug-Runs
ändern die Spawnzeiten, nicht diese nativen Angriffstimer.

Die Map-Tabellen dokumentieren die **nativen Basisschäden vor den neuen Faktoren**.
Für den aktuellen Schaden ist der Faktor aus der folgenden Balancing-Tabelle
zusätzlich anzuwenden. Schaden gilt **pro registriertem Treffer**, bei menschlichem Link,
Enemy Damage ×1 und normalem globalem Schadensmultiplikator, ohne Dodge/Block
oder Rüstungseffekte. 1 Schadenspunkt = ¼ Herz. Größere Elite-Modelle erhöhen
die Trefferflächen, aber nicht automatisch den Angriffsschaden. Zeitabhängige
HP-Skalierung erhöht ebenfalls nicht den Schaden.

Enemy Damage multipliziert den Schaden der zugeordneten Gegner und ihrer
Kind-Aktoren; anschließend können native globale Schadenseinstellungen greifen.
Da Survivors die normale Treffer-Unverwundbarkeit abschaltet, sind diese Werte
nicht als Obergrenze für eine ganze mehrteilige Attacke oder längeren Kontakt
mit einer aktiven Trefferfläche zu verstehen.

## Aktuelles Survivors-Balancing

Alle aktiven Elites/Bosse bewegen sich 50 % schneller; Animationen, Teleports,
Schwerkraft und die Dauer ihrer Trefferfenster werden nicht pauschal beschleunigt.
Normale Gegner, die als Elites/Wächter auftreten, haben halbierte native
Angriffsbereitschafts-/Erholungstimer. Story-Bosse und Zwischenbosse sind davon
ausgenommen. Ganze Animationsabläufe werden nicht halbiert.

| Gegner | Schaden gegenüber nativ | Zusätzliche Regeln |
|---|---|---|
| Beförderte normale Elites/Wächter | ×2 | Bereitschafts-/Erholungstimer ×½; ruhende Wächter bleiben ruhend |
| King Bulblin | ×1 | Nativer Wirbel auch auf Distanz; 6 s Cooldown; aktive Wirbelaxt kann höchstens alle 0,5 s erneut treffen. Normale Schläge senden eine Sword Wave auf Link, Wirbel alle 1 s fünf Wellen (eine auf Links aktuelle Position, vier zufällig gerichtet) (3.000 Einheiten Reichweite, nativer Axt-Basisschaden) |
| Bullbo/Reiter und Elite-Aeralfos | ×8 auf vereinheitlichte native Stärke 1 | Alle Angriffe: 2 Herzen Basisschaden; Schwierigkeit/Endless und Schadensreduktion wirken danach |
| Darknut | ×2 | Wiederholter schwerer Schwertwurf alle 3 aktiven Sekunden bei mehr als 400 Einheiten Abstand, in beiden Rüstungsphasen |
| Death Sword | ×4 | Nach 2,5 s Verfolgung bei mehr als 600 Einheiten Abstand zur nativen Zweifach-Magiephase; laufende Schläge/Schüsse werden nicht neu gestartet |
| Stallord-Kopf | ×4 | Atemstart bis 6.000 Einheiten, unabhängig von Links Schadensschutz; Annäherung ab 1.800 Einheiten |
| Twilit Carrier | ×4 | Ein nativer montierter Bogenschütze; seine Pfeile ebenfalls ×4. Flugpause 2,5 s; Bereitschaft des Bogenschützen halbiert |
| Twilit Bloat | ×3 | Keine lange Angriffssperre mehr; elektrische Vorwarnung, Angriff und Erholung bleiben erhalten |
| Darkhammer | ×3 | Einschließlich Kind-Aktor Kugel; das schadenslose Zurückholen bleibt schadenslos |
| Puppet Zelda | ×4 | Alle drei nativen Angriffstypen |
| Ganondorf | ×5 | Ansturm außerhalb 550 Einheiten aus WAIT, gemeinsamer 5-s-Cooldown auch für native Auslöser; fünf native Geisterreiter alle 8 aktiven Sekunden |
| Phantom Zant | ×5 | Eigene Treffer/Geschosse; beschworene reguläre Gegner behalten ihr eigenes Balancing |

Die ausdrücklich aufgeführten Faktoren **ersetzen** den allgemeinen ×2-Elite-Faktor.
Normale, nicht beförderte Carrier behalten ihren ursprünglichen Schaden und
bekommen keinen zusätzlichen Reiter. Der Difficulty-Regler wird weiter zusätzlich
angewendet. Nullschaden bleibt null. Pfeile speichern den Faktor ihres Ursprungs,
auch wenn der Schütze bereits gestorben ist. Reiter und Geisterreiter werden mit
ihrem Boss entfernt; alle Kind-Aktoren pausieren mit dem Run und werden beim
Run-Ende/Neustart bereinigt. Noch laufende asynchrone Erstellungen bleiben erfasst.

Die folgenden nativen Timerangaben sind Ausgangswerte; für beförderte normale
Gegner gelten die oben genannten Halbierungen. Animationsframes bleiben unverändert.

## Darknut Arena

| Slot | Gegner | Aktive Angriffe und Basisschaden | Timer und Auswahl |
|---|---|---|---|
| 1 | Lizalfos | Kurzer Klingenschlag: ¼ Herz. Zweiteiliger Klingenangriff: je ¼ Herz. Schwanzschlag: ½ Herz. | Gemeinsame Angriffsprüfung alle 30–59 F (1–1,97 s), sofern in Reichweite. Bei freier Bahn 75 % Klinge, sonst Schwanz; kurze/lange Klingenvariante nach Entfernung. Trefferfenster: kurz F22–28; zweiteilig F13–18 und F25–30; Schwanz F33–41. |
| 2 | Helmasaur | Gepanzerter Rammangriff: ¼ Herz. | Feste Zustandsfolge: Vorbereitung → Ausrichten → Ansturm bis 70 F (2,33 s) → Abbremsen. Bei Wandaufprall 50 F (1,67 s) Erholung. Kein zusätzlicher fester Wiederholungs-Cooldown; Survivors aktiviert den nächsten Ansturm erneut. Es ist der vergrößerte normale Helmasaur, nicht die stärkere Helmasaurus-Argumentvariante. |
| 3 | Dynalfos | Zweiteiliger Klingenangriff: je ½ Herz. Schwanzschlag: ¾ Herz. | Gemeinsamer Timer 30–89 F (1–2,97 s). Bei freier Bahn 65 % Klinge, sonst Schwanz. Klinge aktiv F15–20 und F34–43; Schwanz F33–41. Nach Schwanzkontakt kann bei F55 ein Klingennachschlag folgen, ohne neuen normalen Cooldown. |
| 4 | Darknut | Gepanzertes Schwert: 1 Herz; Schildstoß: 0. Schwertwurf: nativ 1 Herz, jetzt zusätzlich wiederholbar. Ungepanzertes Schwert: ½ Herz; Tritt: 0. | Siehe Darknut-Details unten. Phasenwechsel bei 50 % Survivors-HP, keine zufällige Phasenwahl. |

## Hyrule Field

| Slot | Gegner | Aktive Angriffe und Basisschaden | Timer und Auswahl |
|---|---|---|---|
| 1 | Bulblin mit Bogen | Normaler Pfeil: ¼ Herz. | Feste Folge: Bogen spannen → 10–19 F (0,33–0,63 s) zielen → Schuss bei F2 der Schussanimation → erneut spannen oder neu positionieren. Kein separater Cooldown nach dem Schuss. Die Schussvorbereitung gehört zusätzlich zum Abstand. |
| 2 | Bokoblin | Zwei Nahkampfvarianten B/C: jeweils ¼ Herz. | Zufällig 50/50. Nach Rückkehr in FIND zunächst 20 F (0,67 s) Distanz-/Ausrichtungsfenster, danach Angriff bei passender Entfernung. Trefferfenster F14–22. Im nativen kurzen Aggressionszustand kann unmittelbar die nächste Attacke folgen. Kein eigener Cooldown pro Variante. |
| 3 | Kargarok | Anflug-/Schnabelattacke: ¼ Herz. | Anflug → Schweben → Ausfall → Schweben. Angriffssperre `0,5 × (20 + rndF(30))`, ganzzahlig 10–24 F (0,33–0,80 s), zusätzlich muss der Abstand 300–400 Einheiten und die Schwebeanimation weit genug fortgeschritten sein. Trefferfenster F15–27. Zufällige Flugpositionen, keine Rotation verschiedener Schadensfähigkeiten. |
| 4 | King Bulblin | Axtattacke 01 oder 02: jeweils **3 Herzen**. | 20–29 F (0,67–0,97 s) Wartezeit zwischen Angriffsanimationen, in Reichweite 50/50. Trefferfenster 01: F64–72; 02: F62–71. Der native Wirbelangriff ist zusätzlich über den 6-s-Cooldown erreichbar (siehe Balancing). |

## Gerudo Desert

| Slot | Gegner | Aktive Angriffe und Basisschaden | Timer und Auswahl |
|---|---|---|---|
| 1 | Stalhound | Sprung/Biss: ¼ Herz. | Vor dem Sprung 20–34 F (0,67–1,13 s), zusätzlich Ausrichtung, Absprung- und Landeanimation. Feste Folge: Positionieren → vorbereiten → springen/beißen → landen. Zufall beeinflusst die Vorbereitung und Bewegung, nicht die Auswahl mehrerer Attacken. |
| 2 | Stalfos | Schwerer Schwertschlag: ½ Herz. | 10–29 F (0,33–0,97 s) zwischen Angriffsentscheidungen; jede geeignete Entscheidung startet mit 50 % Wahrscheinlichkeit den Schlag. Aktuelle Spawnvariante ist Typ 0 und benutzt ATTACK01, nicht abwechselnd beide Stalfos-Animationen. Animation springt von F10 auf F55, Treffer F81–86, Rückkehr ab F100. |
| 3 | Death Sword | Schwertschlag: ½ Herz; anschließend zwei Magiegeschosse: je ½ Herz. | Feste Abfolge: Distanz suchen → Schwertschlag (F20–33 aktiv) → Erholung → 30 F (1 s) Vorbereitung → Geschosse bei F47 und F67 der Fernkampfanimation, also 20 F (0,67 s) auseinander → erneut Nahkampf. Kein separater wiederkehrender Sekunden-Cooldown für das Schwert. Die ursprüngliche Kreisflugphase wird vom Arena-Adapter übersprungen. |
| 4 | Stallord-Kopf | Feueratem/Feuerprojektil der zweiten Phase: ¾ Herz. | Feste Folge: Ausrichten → 5 F (0,17 s) Timer → Atem-Vorbereitung → ein Projektil → Erholungsanimation, dabei abgesenkt auf Schwertreichweite → erneut warten. Der tatsächliche Schussabstand enthält die vollständigen Vorbereitungs-/Erholungsanimationen; er beträgt nicht 0,17 s. |

## Palace of Twilight

| Slot | Gegner | Aktive Angriffe und Basisschaden | Timer und Auswahl |
|---|---|---|---|
| 1 | Shadow Beast | Zwei Arm-/Nahkampfschläge: je ½ Herz. | 30–59 F (1–1,97 s) vor jeder Attacke. Zufällig etwa ⅓ normale und ⅔ langsame Variante. Trefferfenster F21–30 bzw. F25–33. |
| 2 | Twilit Bloat | Elektrischer Flugansturm: ½ Herz. | Gemeinsame Sperre 300–449 F (10–14,97 s), ab Angriffsbeginn heruntergezählt. Feste Folge Flug → elektrische Vorbereitung (bis Animationsframe 15) → Ansturm → 30 F Erholung → im normalen Survivors-Zustand 110 F (3,67 s) Gelegenheit zum Gegenangriff → Rückflug. Erneuter Ansturm erst nach Ablauf der Sperre und passender Flugposition, insbesondere mehr als 700 Einheiten Abstand. |
| 3 | Twilit Carrier Kargarok | Krallen-/Greifstoß gegen Link: ½ Herz. | Feste angepasste Folge: 150 F (5 s) Flugpause → Annäherung → Abbremsen → 15 F (0,5 s) Vorwarnung → Fanganimation; Treffer bei F11 → zurück zur Flugpause. Beim Elite-Carrier schießt zusätzlich ein montierter Bogenschütze; keine Entführungssequenz. |
| 4 | Phantom Zant | Fächer aus fünf Magiegeschossen: je ½ Herz. Alternativ Portal mit bis zu 20 Shadow Keeses oder 8 Shadow Beasts; Beschwören selbst verursacht 0 Schaden. | Grundentscheidung 40 % Geschossfächer, 60 % Beschwörung. Im Beschwörungszweig wird nativ zufällig unter Typen gewählt; Survivors ersetzt nicht unterstützte Typen durch Shadow Keese und behält Shadow Beast. Deshalb keine einfache garantierte 50/50-Verteilung der beiden Minionarten. Freigabe der Salve bei F23, Beschwörung bei F36. Portal wartet vor den Minions 40 F (1,33 s), danach 2 F zwischen ihnen. Teleportpause 10 F (0,33 s); bestimmte Wartezustände 60 F (2 s). Kein universeller Angriffscooldown: bestehende Minions/Portale und Teleport-/Skalierungsanimationen bestimmen den nächsten Angriff mit. |

Die Gebäudewächter sind zusätzlich platzierte Shadow Bulblins mit Keule.
Sie sind keine zeitgesteuerten Bosse. Der Fix hält ihren gesamten
KI-Aktionsdispatcher an, solange `dormant` gesetzt ist. Trefferverarbeitung,
Animation und Trefferflächen bleiben aktiv. Ein Link-Treffer hebt die Sperre
auf; bloßes Annähern und Geräusche tun dies nicht.

## Hyrule Castle

| Slot | Gegner | Aktive Angriffe und Basisschaden | Timer und Auswahl |
|---|---|---|---|
| 1 | Darknut | Wie Darknut Arena: gepanzertes Schwert und Wurf nativ 1 Herz, ungepanzertes Schwert ½ Herz, Schildstoß/Tritt 0. | Gleiche native Typ-1-KI und Phasen-/Timerregeln, siehe unten. |
| 2 | Darkhammer | Kreisende Kugel und Wurf: je 1 Herz pro Kontakt. Beim eigentlichen Zurückholen ist die Angriffskollision deaktiviert (0 Schaden). | Feste Folge: Anlaufen → Kugel drehen → nach mehr als 80 F (2,67 s) Drehphase und mehr als 200 Einheiten Abstand weitere 40 F (1,33 s) warten → nächstes passendes Animationsfenster → Wurf bei F25 → 100 F (3,33 s) warten → Kugel zurückholen. Rückholen/Animation/Flugstrecke kommen hinzu. Kein zufälliger Wechsel unabhängiger Attacken. |
| 3 | Puppet Zelda | Schwert-Flugstoß ½ Herz; Energiekugel 1 Herz; Dreiecksfeld 1 Herz. | Gemeinsame Pause 100–249 F (3,33–8,30 s). Normalerweise 70 % Kugel, 15 % Schwert, 15 % Dreieck. Nach erfolgreichem Zurückschlagen der Kugel zeitweise nur Schwert/Dreieck 50/50. Kugel: zusätzlich 60 F (2 s) Laden, Freigabe bei F41; Rückschläge laufen als eigene Rally weiter. Schwert: vorbereitende Animationen, dann bis 90 F Verfolgung. Dreieck: erstes Schadensfenster etwa 56 F (1,87 s) nach Einblenden, aktiv bis etwa F84 (2,80 s), Ende nach etwa 160 F (5,33 s). |
| 4 | Ganondorf | Normale Schwertvarianten A/B/F/G, Nahtritt D, Sprungtritt mit Schwertfolge E: je ½ Herz pro Treffer. Schwerer Ansturm C: 1 Herz. | Distanzabhängige Auswahl mit Zufall, siehe unten. Keine feste Rotation. Schwert-Lock/QTE wird von Survivors übersprungen. |

## Darknut: gemeinsame Details beider Maps

Aktueller Spawnparameter `0x000001ff` wählt nativen Typ 1.

* Gepanzert: gemeinsamer Schwerttimer 150 F (5 s). Bei sehr geringem Abstand
  wird er auf 40–59 F (1,33–1,97 s) gekürzt; kurz vor komplettem Rüstungsbruch
  auf 15–34 F (0,5–1,13 s). Das ist ein laufender Timer, keine Zusatzpause
  nach dem Animationsende. Unter 200 Einheiten bevorzugt er den Schildstoß
  ohne diesen Schwerttimer. Schwertvariante A/B überwiegend nach Entfernung,
  am äußeren Rand zufällig 50/50.
* Bei 50 % Run-HP: Rüstung fällt, Rücksprung, einmaliger Schwertwurf bei
  Frame 23 der Wurfanimation, anschließend leichtes Schwert. Zusätzlich läuft der oben beschriebene wiederholbare 3-s-Wurf
  unabhängig von der Rüstungsphase (4 native Punkte vor ×2).
* Ungepanzert: gemeinsamer Timer 350 F (11,67 s), unter 250 Einheiten gekürzt
  auf 60–89 F (2–2,97 s), unter 100 Einheiten auf höchstens 5 F (0,17 s).
  Konter auf Links Aktionen können zusätzliche Übergänge auslösen.
  Stoß-/Schlagwahl nach Entfernung, in manchen Konterzuständen 50/50.
  Der Tritt kann direkt in eine Schwertfolge übergehen (3/5/10 F interne
  Bereitschaft je Folgezustand), statt eine vollständige neue Pause abzuwarten.

## Ganondorf: gemeinsame und individuelle Timer

Nach Eintritt in den nahen Wartestatus: 30–79 F (1–2,63 s). In der entfernten
Annäherung: 200–299 F (6,67–9,97 s), dann gegebenenfalls Ansturm oder Sprung.
Die Erstbegegnung hat außerdem einen 100-F-Sondertimer. Der neue
5-s-Ansturm-Auslöser kann die native entfernte Wartezeit umgehen; die
8-s-Geisterbeschwörung läuft unabhängig von der Schwertauswahl. Die Reichweitenregeln
können die normale nahe Wartezeit umgehen:

| Aktion | Auswahl | Zusätzliches Timing |
|---|---|---|
| A / G | Unter 300 Einheiten, nach gemeinsamem Timer, 50/50 | A trifft F20–28; G hat zwei Fenster F18–24 und F33–40. Je Treffer ½ Herz. |
| B | Mittlere Entfernung, außerdem als Sprung-/Ausweichkonter | F17–23 aktiv; Konter kann bei Animationsframe 10 beginnen. ½ Herz. |
| D: Nahtritt | Unter 150 Einheiten unmittelbar | F3–10 aktiv, kein eigener Zusatz-Cooldown. ½ Herz. |
| F: Schwertfolge nach Sprung | Nahe Landung nach Sprung/Ausweichen | F15–38 aktiv, kein zusätzlicher Warte-Timer nach der Landung. ½ Herz je Kontakt. |
| C: schwerer Ansturm | Auf größere Distanz; im fernen Wartezustand auch Zufallsentscheidung | Nach Vorbereitung bis 50 F (1,67 s) Anlauf, früherer Übergang unter 450 Einheiten. Schwerer Schlag F20–32: 1 Herz. |
| E: Sprungtritt → Schwert | Im nahen Wartestatus über 550 Einheiten 50/50 mit C | 11 F (0,37 s) Vorlauf, Trittfenster F2–12 der Folgeanimation, dann Schwertfenster F10–20 der nächsten Animation; jeweils ½ Herz. |

## Quellstellen

Mod: `src/survivors_roster.hpp`, `src/survivors_roster_native.inc`,
`src/survivors_elites.inc`, `src/survivors_bosses.inc`,
`src/survivors_encounters.inc`, `src/survivors_boss_specials.inc`,
`src/survivors_enemy_balance.inc`, `src/survivors_enemy_arrows.inc`, `src/survivors_extended_ai.inc`,
`src/survivors_extended_damage.inc`, `src/survivors_options.inc`,
`src/survivors_player_damage.inc` und `src/survivors_defense.inc`.

SDK: `src/d/actor/d_a_e_{dn,mm,mm_mt,mf,rd,arrow,oc,kr,rdb,sh,sf,vt,s1,ymb,yc,pz,th,th_ball,hzelda}.cpp`,
`d_a_b_{tn,ds,gnd}.cpp`; dort die Action-/Attack-Funktionen und Colliderquellen.
Schadensweg: `src/SSystem/SComponent/c_cc_s.cpp::CalcTgPlusDmg` und
`src/d/actor/d_a_alink_damage.inc::{checkDamageAction,setDamagePoint,damageMagnification}`.
