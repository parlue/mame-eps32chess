// Oeffentliche Schnittstelle der MAME-0.220-Komponente fuer den ESP32-S3.
//
// Meilenstein 0: Selbsttest der kompilierten MAME-Utilities.
// Meilenstein 1+: hier kommen Start/Stop der Emulation und die Zugbruecke dazu.
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Fuehrt echte MAME-0.220-Utility-Funktionen aus. Rueckgabe 1 = OK, 0 = Fehler.
int mame_bridge_selfcheck(void);

// Kurzbeschreibung des Builds (Quellstand/Tag).
const char *mame_bridge_note(void);

#ifdef __cplusplus
}
#endif
