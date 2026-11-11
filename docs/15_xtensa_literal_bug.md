# Xtensa: falscher Stringzeiger durch Linker-Relaxierung

Nachgewiesen mit der tatsächlich verwendeten Toolchain
`esp-13.2.0_20240530` und MAME 0.220. Keine Änderung der Original-MAME-Quellen.

## Befund am Gerät

`takeover-uci-04.log` endet mit `std::logic_error`, weil config.cpp:85 einen
Nullzeiger in einen C++-String umwandelt. Der Throw-Backtrace führt in
`configuration_manager::load_settings()`. Die Disassemblierung des fertigen
Build-09-Binarys zeigt beim Aufruf von `core_options::value` den Namen
`default.cfg`; im Originalcode und in der Objektdatei steht `cfg_directory`.
Die nicht vorhandene Option erklärt den Nullzeiger.

## Ursache und Reproduktion

MAMEs Formatargumente enthalten Anfangs- und Endzeiger auf Stringliterale.
Ein Endzeiger hinter dem Nullzeichen kann im ursprünglichen Stringpool
numerisch gleich dem Anfang des folgenden Literals sein. Der Xtensa-Linker
fasst die dazugehörigen Literalplätze zusammen. Werden gleiche Zeichenketten
aus mehreren Objektdateien anschließend an andere Adressen zusammengelegt,
ist die Gleichheit nicht mehr gegeben. Der gemeinsam verwendete Literalplatz
kann dann einen falschen Stringanfang liefern.

Der unabhängige Test benutzt nur zwei kleine C++-Dateien, ohne MAME oder ESP-IDF:

```powershell
python tools/test_xtensa_literals.py
```

Der Test kompiliert und linkt mit der vorhandenen echten S3-Toolchain. Er liest
die ELF-Sektionen und zählt die tatsächlich eingebetteten korrekten Zeiger auf
`ctrlr` (ein Zugriff aus jeder Objektdatei). Ergebnis am 10. November 2026:

```text
default: 1 correct ctrlr references (expected 2)
no-merge: 2 correct ctrlr references (expected 2)
PASS: targeted no-merge fix; default linker bug reproduced
```

Die Disassemblierungen liegen in `xtensa-literal-probe.txt`,
`xtensa-literal-no-relax.txt` und `xtensa-literal-no-merge.txt` im docs-Ordner.

## Gewählte Korrektur

Die MAME-Komponente erhält `-fno-merge-constants`. Damit bleiben ihre
Stringabschnitte zusammen und die Voraussetzung für das falsche Zusammenlegen
der Literalplätze entfällt. SDK und normale Linker-Relaxierung bleiben aktiv.
Es werden keine CPU-Kerne, Geräte oder Emulationsfunktionen entfernt.

Globales `--no-relax` korrigiert das isolierte Beispiel ebenfalls, scheitert
aber beim Firmware-Link an acht IDF-Ausnahme-/Interruptvektoren, deren Literale
der Linker verschieben muss. Build 10 wurde deshalb nicht geflasht. Das
Post-Skript `no_relax_link.py` ist ausschließlich ein archivierter Diagnoseweg.

Build 11 hat die Compilerkorrektur im vollständigen Binary bestätigt:
`config-build11-disassembly.txt`, Aufruf bei 0x42019263, lädt cfg_directory
von 0x3c61e5a5. default.cfg liegt separat bei 0x3c61e5b3. Der Link ist erfolgreich,
Flashbedarf 9.138.232 Bytes. PlatformIO scheiterte erst beim Drucken des nächsten
Buildbefehls im Verbose-Modus (SCons `_Null` + str); die Flash-Datei wurde mit
esptool 5.1 direkt aus genau diesem ELF erfolgreich erstellt.

Der tatsächliche ROM-/UCI-Start auf dem Gerät muss zusätzlich bestätigt werden.
Ein bestandenes Minimalbeispiel und ein korrektes Binary allein sind noch kein
erfolgreicher Emulationsstart.
