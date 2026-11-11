// ---------------------------------------------------------------------------
// xtensa_posix_shim.h - Ersatz-DEKLARATION fuer genau eine libc-Funktion, die
// auf xtensa/newlib fehlt, aber beim Uebersetzen von MAMEs HTTP-Verwalter
// (src/emu/http.cpp -> asio, Header-only) deklariert sein muss:
//
//     int socketpair(int, int, int, int[2]);
//
// Grund (Lauf #24): Der Linker verlangte http_manager::clear(),
// add_http_handler() und ~http_manager(), weil running_machine::run()
// bedingungslos m_manager.http()->clear() aufruft (machine.cpp:306/383). Damit
// muss src/emu/http.cpp uebersetzt werden. Der Uebersetzer von asio
// (3rdparty/asio/include/asio/detail/impl/socket_ops.ipp:623) uebersetzt die
// Inline-Funktion socket_ops::socketpair, die ::socketpair aufruft; lwIP/IDF
// hat socketpair nicht (in lwip/sockets.h geprueft). Da asio die Funktion
// ausschliesslich inline definiert (ASIO_HEADER_ONLY -> ASIO_DECL inline,
// asio/detail/config.hpp:34-43) und sie nirgends aufgerufen wird, genuegt die
// Deklaration: es entsteht kein Aufruf und damit kein undefiniertes Symbol.
//
// Das ist KEIN Ersatzverhalten: die Datei enthaelt nur eine Deklaration, keine
// Definition, und aendert keine MAME-Quelle. Der HTTP-Server bleibt auf dem S3
// inaktiv (options().http() ist aus, http.cpp:276 kehrt sofort zurueck).
//
// Die Datei wird der Komponente per -include vorgegeben (siehe CMakeLists.txt).
// ---------------------------------------------------------------------------

#ifndef MAME_S3_XTENSA_POSIX_SHIM_H
#define MAME_S3_XTENSA_POSIX_SHIM_H

// Zusatz (Lauf #24, Fehler 2): pugixml.cpp:4564 verlangt LLONG_MIN/LLONG_MAX/
// ULLONG_MAX. Sie stehen in <limits.h> (C) bzw. <climits> (C++); auf xtensa
// fehlen sie je nach Spracheinstellung. Diese Datei wird jeder
// Uebersetzungseinheit der Komponente per -include vorangestellt und stellt sie
// deshalb fuer alle bereit. Die #ifndef-Rueckfallwerte greifen nur, wenn der
// Header sie nicht liefert - die Quellen bleiben unveraendert.
#include <limits.h>
#ifndef LLONG_MAX
#define LLONG_MAX 9223372036854775807LL
#endif
#ifndef LLONG_MIN
#define LLONG_MIN (-LLONG_MAX - 1LL)
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX 18446744073709551615ULL
#endif

#ifdef __cplusplus
extern "C" {
#endif

int socketpair(int domain, int type, int protocol, int sv[2]);

#ifdef __cplusplus
}
#endif

#endif // MAME_S3_XTENSA_POSIX_SHIM_H
