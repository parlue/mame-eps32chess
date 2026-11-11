// ---------------------------------------------------------------------------
// s3_startup.cpp - Startpfad der Emulation auf dem ESP32-S3.
//
// Was MAME sonst selbst mitbringt und hier ersetzt wird:
//
//   * die Treiberliste. MAME erzeugt sie zur Bauzeit aus src/mame/mame.lst
//     (Datei drivlist.cpp: Tabelle aller Treiber). Aus dem riesigen MAME-Baum
//     wird hier genau ein Treiber gebaut - mm4 (Mephisto MM IV). Die Tabelle
//     enthaelt deshalb genau diesen Eintrag. Der Treiber selbst liegt
//     unveraendert in src/mame/drivers/mephisto_mm2.cpp.
//
//   * src/emu/main.cpp (MAMEs eigenes main()). Es wird nicht gebraucht: der
//     Einstiegspunkt des S3 ist app_main() in src/main.c, das mame_s3_run()
//     aufruft.
//
//   * src/frontend/mame/* (Menues, Kommandozeile). Wird nicht gebraucht:
	//     Der Kern erhaelt die minimale ui_manager-Basisklasse, weil auch
	//     ioport/video ohne sichtbares Menue auf sie zugreifen.
// ---------------------------------------------------------------------------

#include "emu.h"
#include "emuopts.h"
#include "drivenum.h"
#include "mconfig.h"
#include "s3_osd.h"
#include "ui/uimain.h"
#include "sound.h"
#include "esp_heap_caps.h"
#include "esp_debug_helpers.h"
#include "esp_rom_sys.h"
#include "../../../../integration/chessengine/include/s3_uci_console.h"

#include <cstdio>
#include <string>
#include <cstring>

// MAME catches std::exception internally, so the ordinary error log has already
// lost the throwing stack. Preserve the precise site of the startup logic_error.
extern "C" [[noreturn]] void __real___cxa_throw(void *, void *, void (*)(void *));
extern "C" [[noreturn]] void __wrap___cxa_throw(void *exception, void *info, void (*destroy)(void *))
{
	const char *name = static_cast<std::type_info *>(info)->name();
	if (std::strstr(name, "logic_error")) {
		esp_rom_printf("[CXX THROW] %s\n", name);
		esp_backtrace_print(20);
	}
	__real___cxa_throw(exception, info, destroy);
}

//--------------------------------------------------------------------------
// MAMEs Bau erzeugt normalerweise eine Datei version.cpp mit diesen beiden
// Zeichenketten (makefile: 'extern const char build_version[];' ...). Da unser
// Bau diesen Schritt nicht hat, stehen sie hier.
//--------------------------------------------------------------------------

const char build_version[] = "0.220 (unveraenderte Originalquellen, ESP32-S3)";
const char bare_build_version[] = "0.220";

//--------------------------------------------------------------------------
// emulator_info - MAME fuellt diese Klasse in src/frontend/mame/mame.cpp
// (Zeilen 375-432). Die Frontend-Ebene wird hier bewusst nicht gebaut (keine
// Menues, keine Kommandozeile, keine Grafikausgabe). Die vom Kern und vom
// Grafikgeruest aufgerufenen Funktionen sind deshalb hier ersetzt; die
// Fassungen ohne Vorbild im Frontend sind wirkungslos, weil auf dem S3 weder
// Fenster noch Menue noch Tonausgabe existieren. Keine MAME-Quelle geaendert.
//--------------------------------------------------------------------------

const char *emulator_info::get_appname() { return "MAME"; }
const char *emulator_info::get_appname_lower() { return "mame"; }
const char *emulator_info::get_configname() { return "mame"; }
const char *emulator_info::get_copyright() { return "Copyright Nicola Salmoria and the MAME Team"; }
const char *emulator_info::get_copyright_info() { return "MAME ist unter der BSD-3-Clause-Lizenz veroeffentlicht"; }
const char *emulator_info::get_bare_build_version() { return bare_build_version; }
const char *emulator_info::get_build_version() { return build_version; }
void emulator_info::display_ui_chooser(running_machine &machine) { }
int emulator_info::start_frontend(emu_options &options, osd_interface &osd, std::vector<std::string> &args) { return EMU_ERR_NONE; }
int emulator_info::start_frontend(emu_options &options, osd_interface &osd, int argc, char *argv[]) { return EMU_ERR_NONE; }
void emulator_info::draw_user_interface(running_machine &machine) { }
void emulator_info::periodic_check() { }
bool emulator_info::frame_hook() { return false; }
void emulator_info::sound_hook() { }
void emulator_info::layout_file_cb(util::xml::data_node const &layout) { }
bool emulator_info::standalone() { return false; }

//--------------------------------------------------------------------------
// Treibertabelle (Ersatz fuer die erzeugte drivlist.cpp)
//--------------------------------------------------------------------------

GAME_EXTERN(mm4);
GAME_EXTERN(mm5);

game_driver const *const driver_list::s_drivers_sorted[] = { &GAME_NAME(mm4), &GAME_NAME(mm5) };
std::size_t const driver_list::s_driver_count = 2;

//--------------------------------------------------------------------------
// Maschinenverwaltung: minimale Ableitung von machine_manager
//
// machine_manager (src/emu/main.h) verlangt nur einen Konstruktor mit
// (emu_options &, osd_interface &); create_ui()/create_custom() sind bereits
// mit leeren Vorgaben vorhanden. Damit braucht der S3 kein Frontend.
//--------------------------------------------------------------------------

class s3_machine_manager : public machine_manager
{
public:
	s3_machine_manager(emu_options &options, osd_interface &osd)
		: machine_manager(options, osd)
	{
	}

	virtual ~s3_machine_manager() = default;
	void release_ui() { m_headless_ui.reset(); }
	void create_custom(running_machine &machine) override
	{
		std::printf("[MAME PRE-DEVICE] sample_rate=%d update_attoseconds=%lld heap=%u psram=%u largest=%u\n",
			machine.sample_rate(), (long long)machine.sound().update_attoseconds(),
			unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
			unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
			unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)));
	}

	ui_manager *create_ui(running_machine &machine) override
	{
		// Even a headless machine calls ui().is_menu_active() in ioport.cpp
		// and ui().show_timecode_counter() in video.cpp. The core base class
		// supplies the proper no-menu implementation; nullptr would crash.
		m_headless_ui = std::make_unique<ui_manager>(machine);
		return m_headless_ui.get();
	}

private:
	std::unique_ptr<ui_manager> m_headless_ui;
};

// Lauf #24: MAMEs src/emu/main.cpp wird jetzt mituebersetzt (siehe
// CMakeLists.txt). Sie enthaelt ausschliesslich den Konstruktor von
// machine_manager und start_http_server() - kein main(). Die frueher hier
// noetige Ersatzkopie des Konstruktors ist deshalb entfallen (sie haette sonst
// ein doppeltes Symbol ergeben).

//--------------------------------------------------------------------------
// Ausgabe: MAMEs osd_printf_* auf die Konsole (UART)
//
// MAME verteilt alle Meldungen ueber eine Kette von osd_output-Objekten
// (src/osd/osdcore.cpp). Ohne Eintrag in dieser Kette waeren alle Ausgaben
// leer - deshalb haengt sich dieses Objekt ein.
//--------------------------------------------------------------------------

namespace {

class s3_output : public osd_output
{
public:
	void output_callback(osd_output_channel channel, util::format_argument_pack<std::ostream> const &args) override
	{
		const std::string text = util::string_format(args);

		switch (channel)
		{
		case OSD_OUTPUT_CHANNEL_ERROR:
			std::fprintf(stderr, "MAME-Fehler: %s\n", text.c_str());
			break;
		case OSD_OUTPUT_CHANNEL_WARNING:
			std::fprintf(stderr, "MAME-Warnung: %s\n", text.c_str());
			break;
		case OSD_OUTPUT_CHANNEL_INFO:
		case OSD_OUTPUT_CHANNEL_VERBOSE:
		case OSD_OUTPUT_CHANNEL_LOG:
			std::printf("MAME: %s\n", text.c_str());
			break;
		case OSD_OUTPUT_CHANNEL_DEBUG:
			// Debug-Ausgaben nur mit Schalter, sonst rauscht der UART.
			break;
		default:
			break;
		}

		chain_output(channel, args);
	}
};

s3_output g_output;

} // anonymer Namensraum

//--------------------------------------------------------------------------
// oeffentliche Einstiegspunkte (von src/main.c aus aufgerufen)
//--------------------------------------------------------------------------

extern "C" const char *mame_s3_target(void)
{
	// Nennt Treiber und Bauversion. Der Treibername kommt aus der Tabelle
	// oben - damit ist belegt, dass der Treiber im Abbild steckt.
	static std::string text;
	if (text.empty())
	{
		const int index = driver_list::find("mm4");
		text = (index >= 0)
			? (std::string(build_version) + " / " + driver_list::driver(index).name)
			: std::string(build_version) + " / Treiber mm4 fehlt";
	}
	return text.c_str();
}

extern "C" int mame_s3_run_driver(const char *name)
{
	if (!name || !*name) return -1;
	osd_output::push(&g_output);

	std::printf("--- MAME %s auf ESP32-S3 ---\n", build_version);

	const int index = driver_list::find(name);
	if (index < 0)
	{
		std::printf("Treiber %s nicht in der Tabelle\n", name);
		osd_output::pop(&g_output);
		return -1;
	}
	const game_driver &driver = driver_list::driver(index);
	std::printf("Treiber: %s\n", driver.name);

	static emu_options options;
	static s3_osd osd;
	static s3_machine_manager manager(options, osd);

	// Lauf #24: running_machine::run() ruft bedingungslos
	// m_manager.http()->clear() auf (machine.cpp:306 und :383). machine_manager
	// legt den Verwalter nur in start_http_server() an (src/emu/main.cpp:21-24),
	// und im Original ruft das die Frontend-Ebene. Ohne diesen Aufruf waere
	// http() nullptr und der S3 wuerde beim Maschinenstart abstuerzen.
	// options().http() ist per Vorgabe aus, der Verwalter also inaktiv:
	// http.cpp:276 kehrt sofort zurueck, clear() ebenso - es startet kein
	// Server, kein Thread, kein Socket.
	manager.start_http_server();

	// Suchpfad fuer die ROM-Dateien: /roms im Dateisystem des VFS
	// (LittleFS/SPIFFS-Partition bzw. SD-Karte, siehe docs/12).
	// Die Prioritaet ist bei core_options::set_value Pflicht (options.h:195).
	options.set_value(std::string(OPTION_MEDIAPATH), std::string("/roms"), OPTION_PRIORITY_CMDLINE);
	options.set_value(std::string(OPTION_CFG_DIRECTORY), std::string("/roms/cfg"), OPTION_PRIORITY_CMDLINE);
	options.set_value(std::string(OPTION_NVRAM_DIRECTORY), std::string("/roms/nvram"), OPTION_PRIORITY_CMDLINE);
	options.set_value(std::string(OPTION_STATE_DIRECTORY), std::string("/roms/sta"), OPTION_PRIORITY_CMDLINE);
	options.set_value(std::string(OPTION_INIPATH), std::string("/roms/ini"), OPTION_PRIORITY_CMDLINE);
	options.set_value(std::string(OPTION_SKIP_GAMEINFO), true, OPTION_PRIORITY_CMDLINE);
	// sound_manager's no_sound path selects 11025 Hz only AFTER constructing
	// its three mix vectors. Set the same effective rate beforehand so they
	// do not retain 48000-entry buffers on the 8 MiB target (369750 bytes).
	options.set_value(std::string(OPTION_SAMPLERATE), 11025, OPTION_PRIORITY_CMDLINE);
	// The original frontend does this before constructing running_machine.
	// It also discovers the driver's image/slot options (including sensorboard).
	// Without it image_manager dereferences an absent image_option at startup.
	options.set_system_name(std::string(driver.name));

	int result;
	{
		machine_config config(driver, options);
		running_machine machine(config, manager);
		manager.set_machine(&machine);
		std::printf("starte Maschine...\n");
		result = machine.run(true);
		mame_s3_uci_close();
		manager.release_ui();
	} // Destroy the old machine before accepting another Load request.
	manager.set_machine(nullptr);
	std::printf("Maschine beendet, Ergebnis %d\n", result);

	osd_output::pop(&g_output);
	return result;
}

extern "C" int mame_s3_run(void)
{
	return mame_s3_run_driver("mm4");
}
