// ---------------------------------------------------------------------------
// s3_osd_core.cpp - die freien OSD-Funktionen aus src/osd/osdcore.h.
//
// MAME 0.220 fordert diese Funktionen von jeder Plattform (Windows:
// src/osd/windows, SDL: src/osd/sdl). Hier werden sie fuer den ESP32-S3
// bereitgestellt:
//
//   Datei-E/A   ueber das ESP-IDF-VFS (stdio), damit ROM-Dateien aus einer
//               Flash-Partition gelesen werden koennen
//   Zeit        ueber esp_timer_get_time() (Mikrosekunden)
//   Verzeichnis ueber opendir/readdir des VFS
//   Threading   NICHT vorhanden: die Arbeitsschlangen (osd_work_*) fuehren die
//               Arbeit sofort im aufrufenden Kontext aus. Das ist im Quelltext
//               ausdruecklich vorgesehen ("If no threading support is
//               available, it is a simple matter to execute the work items as
//               they are queued", osdcore.h).
//
// Was absichtlich fehlt: Fenster, Sound, MIDI, Netzwerk-Adapterliste,
// Zwischenablage, ausfuehrbarer Speicher fuer JIT-Backends (auf xtensa gibt es
// keinen JIT; die Funktionen liefern trotzdem gueltigen Speicher).
// ---------------------------------------------------------------------------

#include "osdcore.h"
#include "osdepend.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <new>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_timer.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//==========================================================================
//  DATEI-EIN-/AUSGABE
//==========================================================================

namespace {

// Konkrete Datei auf dem VFS. Die Schnittstelle von osd_file ist
// offset-basiert (read(buffer, offset, length, actual)) - deshalb wird vor
// jedem Zugriff positioniert.
class s3_file : public osd_file
{
public:
	s3_file(std::FILE *f, std::uint64_t size) : m_file(f), m_size(size) { }
	virtual ~s3_file() override { if (m_file) std::fclose(m_file); }

	error read(void *buffer, std::uint64_t offset, std::uint32_t length, std::uint32_t &actual) override
	{
		actual = 0;
		if (!m_file || offset > m_size) return error::FAILURE;
		if (std::fseek(m_file, static_cast<long>(offset), SEEK_SET) != 0) return error::FAILURE;
		size_t got = std::fread(buffer, 1, length, m_file);
		actual = static_cast<std::uint32_t>(got);
		return std::ferror(m_file) ? error::FAILURE : error::NONE;
	}

	error write(void const *buffer, std::uint64_t offset, std::uint32_t length, std::uint32_t &actual) override
	{
		actual = 0;
		if (!m_file) return error::INVALID_ACCESS;
		if (std::fseek(m_file, static_cast<long>(offset), SEEK_SET) != 0) return error::FAILURE;
		size_t put = std::fwrite(buffer, 1, length, m_file);
		actual = static_cast<std::uint32_t>(put);
		if (offset + put > m_size) m_size = offset + put;
		return (put == length) ? error::NONE : error::FAILURE;
	}

	error truncate(std::uint64_t offset) override
	{
		// Kein ftruncate im IDF-VFS: nur der Puffer wird geleert. Speicher-
		// staende (nvram) werden in dieser Fassung nicht geschrieben.
		(void)offset;
		if (!m_file) return error::INVALID_ACCESS;
		std::fflush(m_file);
		return error::NONE;
	}

	error flush() override
	{
		if (!m_file) return error::INVALID_ACCESS;
		return (std::fflush(m_file) == 0) ? error::NONE : error::FAILURE;
	}

private:
	std::FILE *m_file;
	std::uint64_t m_size;
};

} // anonymer Namensraum

osd_file::error osd_file::open(std::string const &path, std::uint32_t openflags, ptr &file, std::uint64_t &filesize)
{
	file.reset();
	filesize = 0;

	// Zugriffsmodus aus den OPEN_FLAG_*-Bits bilden
	std::string mode;
	const bool rd = (openflags & OPEN_FLAG_READ) != 0;
	const bool wr = (openflags & OPEN_FLAG_WRITE) != 0;
	const bool cr = (openflags & OPEN_FLAG_CREATE) != 0;

	if (wr)      mode = cr ? "w+b" : "r+b";
	else if (rd) mode = "rb";
	else         mode = "rb";

	// OPEN_FLAG_CREATE_PATHS wird nicht unterstuetzt: das VFS legt keine
	// Verzeichnisse an. Bereichsangaben wie "roms" muessen vorhanden sein.
	std::FILE *f = std::fopen(path.c_str(), mode.c_str());
	if (!f)
		return (errno == ENOENT) ? error::NOT_FOUND : error::ACCESS_DENIED;

	std::fseek(f, 0, SEEK_END);
	long size = std::ftell(f);
	std::fseek(f, 0, SEEK_SET);
	if (size < 0) size = 0;

	filesize = static_cast<std::uint64_t>(size);
	file = std::make_unique<s3_file>(f, filesize);
	return error::NONE;
}

osd_file::error osd_file::openpty(ptr &file, std::string &name)
{
	// Keine Pseudoterminals auf dem S3 (dipty.cpp wird nicht gebraucht).
	file.reset();
	name.clear();
	return error::FAILURE;
}

osd_file::error osd_file::remove(std::string const &filename)
{
	return (std::remove(filename.c_str()) == 0) ? error::NONE : error::FAILURE;
}

//==========================================================================
//  VERZEICHNISSE
//==========================================================================

namespace osd {

namespace {

class s3_directory : public directory
{
public:
	explicit s3_directory(DIR *dir) : m_dir(dir) { }

	virtual ~s3_directory() override
	{
		if (m_dir) ::closedir(m_dir);
	}

	virtual const entry *read() override
	{
		if (!m_dir) return nullptr;
		struct dirent *d = ::readdir(m_dir);
		if (!d) return nullptr;
		m_name = d->d_name;
		m_entry.name = m_name.c_str();
		m_entry.type = (d->d_type == DT_DIR) ? entry::entry_type::DIR
			: (d->d_type == DT_REG) ? entry::entry_type::FILE
			: entry::entry_type::OTHER;
		m_entry.size = 0;
		m_entry.last_modified = std::chrono::system_clock::time_point();
		return &m_entry;
	}

private:
	DIR *m_dir;
	std::string m_name;
	entry m_entry;
};

} // anonymer Namensraum

directory::ptr directory::open(std::string const &dirname)
{
	DIR *dir = ::opendir(dirname.c_str());
	if (!dir) return nullptr;
	return std::make_unique<s3_directory>(dir);
}

} // namespace osd

//==========================================================================
//  ZEIT
//==========================================================================

osd_ticks_t osd_ticks()
{
	// esp_timer liefert Mikrosekunden seit dem Einschalten (64 Bit).
	return static_cast<osd_ticks_t>(esp_timer_get_time());
}

osd_ticks_t osd_ticks_per_second()
{
	return 1000000;      // 1 Tick = 1 Mikrosekunde
}

void osd_sleep(osd_ticks_t duration)
{
	const int64_t us = static_cast<int64_t>(duration);
	if (us <= 0) return;
	TickType_t ticks = pdMS_TO_TICKS(static_cast<uint32_t>(us / 1000));
	if (ticks == 0) ticks = 1;
	vTaskDelay(ticks);
}

//==========================================================================
//  UMGEBUNG, PROZESS, PFADE
//==========================================================================

const char *osd_getenv(const char *name)
{
	// newlib liefert getenv(); auf dem S3 sind keine Variablen gesetzt, der
	// Aufruf ist trotzdem korrekt und liefert nullptr.
	return std::getenv(name);
}

int osd_getpid()
{
	return 1;   // ein Prozess, ein Kern-Ablauf
}

bool osd_get_physical_drive_geometry(const char *filename, uint32_t *cylinders, uint32_t *heads, uint32_t *sectors, uint32_t *bps)
{
	// Keine physischen Laufwerke (CHD-Images werden nicht gebraucht).
	(void)filename; (void)cylinders; (void)heads; (void)sectors; (void)bps;
	return false;
}

int osd_uchar_from_osdchar(char32_t *uchar, const char *osdchar, size_t count)
{
	// Die OSD-Zeichen kommen hier als UTF-8 an (MAMEs Suchmuster und Pfade
	// sind UTF-8). Es wird genau ein Zeichen dekodiert.
	if (!uchar || !osdchar || !count) return 0;

	const unsigned char c0 = static_cast<unsigned char>(osdchar[0]);
	size_t need = 0;
	char32_t cp = 0;

	if (c0 < 0x80)             { need = 1; cp = c0; }
	else if ((c0 & 0xe0) == 0xc0) { need = 2; cp = c0 & 0x1f; }
	else if ((c0 & 0xf0) == 0xe0) { need = 3; cp = c0 & 0x0f; }
	else if ((c0 & 0xf8) == 0xf0) { need = 4; cp = c0 & 0x07; }
	else return 0;                 // ungueltiges Startbyte

	if (need > count) return 0;

	for (size_t i = 1; i < need; ++i)
	{
		const unsigned char ci = static_cast<unsigned char>(osdchar[i]);
		if ((ci & 0xc0) != 0x80) return 0;
		cp = (cp << 6) | (ci & 0x3f);
	}

	*uchar = cp;
	return static_cast<int>(need);
}

bool osd_is_valid_filename_char(char32_t uchar)
{
	// Zulaessig wie unter Windows, aber ohne die dortigen Sonderfaelle:
	// Steuerzeichen, Pfadtrenner und die bekannten verbotenen Zeichen.
	if (uchar < 32) return false;
	switch (uchar)
	{
	case '\\': case '/': case ':': case '*': case '?':
	case '"':  case '<': case '>': case '|':
		return false;
	default:
		return true;
	}
}

bool osd_is_valid_filepath_char(char32_t uchar)
{
	// Im Pfad sind die Trenner erlaubt.
	if (uchar < 32) return false;
	switch (uchar)
	{
	case ':': case '*': case '?': case '"': case '<': case '>': case '|':
		return false;
	default:
		return true;
	}
}

bool osd_is_absolute_path(const std::string &path)
{
	if (path.empty()) return false;
	// "/roms" ist absolut; "roms/..." haengt am Suchpfad. Ein Laufwerksbuch-
	// stabe wie "C:" ist auf dem S3 bedeutungslos, wird aber erkannt.
	if (path[0] == '/' || path[0] == '\\') return true;
	return (path.size() >= 2) && (path[1] == ':');
}

//==========================================================================
//  ARBEITSSCHLANGEN
//
// Auf dem S3 laeuft die Emulation in einem FreRTOS-Task. Arbeitsschlangen
// werden deshalb sofort im aufrufenden Kontext ausgefuehrt (siehe Kopf dieser
// Datei). Damit ist das Verhalten deterministisch - fuer eine Emulation sogar
// guenstiger als echte Nebenlaeufigkeit.
//==========================================================================

struct osd_work_queue
{
	int flags;
};

struct osd_work_item
{
	void *result;
};

osd_work_queue *osd_work_queue_alloc(int flags)
{
	osd_work_queue *q = new (std::nothrow) osd_work_queue;
	if (q) q->flags = flags;
	return q;
}

int osd_work_queue_items(osd_work_queue *queue)
{
	(void)queue;
	return 0;      // es gibt nie offene Arbeit: sie laeuft sofort
}

bool osd_work_queue_wait(osd_work_queue *queue, osd_ticks_t timeout)
{
	(void)queue; (void)timeout;
	return true;
}

void osd_work_queue_free(osd_work_queue *queue)
{
	delete queue;
}

osd_work_item *osd_work_item_queue_multiple(osd_work_queue *queue, osd_work_callback callback, int32_t numitems, void *parambase, int32_t paramstep, uint32_t flags)
{
	(void)queue;
	if (!callback || (numitems <= 0)) return nullptr;

	osd_work_item *last = nullptr;
	for (int32_t i = 0; i < numitems; ++i)
	{
		void *param = static_cast<std::uint8_t *>(parambase) + (static_cast<std::ptrdiff_t>(i) * paramstep);
		void *result = callback(param, 0);

		if ((flags & WORK_ITEM_FLAG_AUTO_RELEASE) != 0)
			continue;               // Ergebnis wird nicht gebraucht

		osd_work_item *item = new (std::nothrow) osd_work_item;
		if (item)
		{
			item->result = result;
			last = item;
		}
	}
	return last;
}

bool osd_work_item_wait(osd_work_item *item, osd_ticks_t timeout)
{
	// Die Arbeit ist beim Erzeugen des Elements bereits erledigt.
	(void)item; (void)timeout;
	return true;
}

void *osd_work_item_result(osd_work_item *item)
{
	return item ? item->result : nullptr;
}

void osd_work_item_release(osd_work_item *item)
{
	delete item;
}

//==========================================================================
//  SONSTIGES
//==========================================================================

void *osd_alloc_executable(size_t size)
{
	// Auf xtensa wird kein JIT benutzt (MAME_NOASM/MAME_NOJIT); der Aufruf
	// muss aber gueltigen Speicher liefern.
	return std::malloc(size);
}

void osd_free_executable(void *ptr, size_t size)
{
	(void)size;
	std::free(ptr);
}

void osd_break_into_debugger(const char *message)
{
	// Kein Debugger: die Meldung geht auf die Konsole (UART).
	if (message)
		std::fprintf(stderr, "MAME break: %s\n", message);
}

std::string osd_get_clipboard_text()
{
	return std::string();      // keine Zwischenablage
}

osd_file::error osd_get_full_path(std::string &dst, std::string const &path)
{
	// Es gibt kein Arbeitsverzeichnis im POSIX-Sinn: ein relativer Pfad wird
	// als absolut unter "/" gefuehrt. MAME benutzt das Ergebnis zum Vergleich
	// von Pfaden, nicht zum Oeffnen.
	if (osd_is_absolute_path(path))
		dst = path;
	else
		dst = std::string(PATH_SEPARATOR) + path;
	return osd_file::error::NONE;
}

namespace {

// Der Eintrag haelt nur einen Zeiger auf den Namen; deshalb muss der Name
// laenger leben als das Ergebnis. Bei einem Aufruf pro Pruefung ist ein
// statischer Puffer ausreichend (die Emulation laeuft in einem Task).
std::string s_stat_name;

} // anonymer Namensraum

std::unique_ptr<osd::directory::entry> osd_stat(std::string const &path)
{
	struct stat st;
	if (::stat(path.c_str(), &st) != 0)
		return nullptr;

	s_stat_name = path;

	auto result = std::make_unique<osd::directory::entry>();
	result->name = s_stat_name.c_str();
	result->type = S_ISDIR(st.st_mode) ? osd::directory::entry::entry_type::DIR
		: S_ISREG(st.st_mode) ? osd::directory::entry::entry_type::FILE
		: osd::directory::entry::entry_type::OTHER;
	result->size = static_cast<std::uint64_t>(st.st_size);
	result->last_modified = std::chrono::system_clock::from_time_t(st.st_mtime);
	return result;
}

void osd_list_network_adapters()
{
	// Keine Netzwerkausgabe auf dem S3.
}

const char *osd_get_volume_name(int idx)
{
	(void)idx;
	return nullptr;
}

void osd_subst_env(std::string &dst, std::string const &src)
{
	// Keine Umgebungsvariablen auf dem S3: der Pfad bleibt unveraendert.
	dst = src;
}

void osd_set_aggressive_input_focus(bool aggressive_focus)
{
	(void)aggressive_focus;
}

std::vector<std::string> osd_get_command_line(int argc, char *argv[])
{
	// MAME wird hier programmatisch gestartet, nicht ueber eine Kommandozeile.
	(void)argc; (void)argv;
	return std::vector<std::string>();
}
