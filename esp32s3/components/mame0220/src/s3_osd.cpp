// ---------------------------------------------------------------------------
// s3_osd.cpp - osd_interface fuer ESP32-S3.
//
// MAME ruft diese Methoden an festen Punkten des Emulationslaufs. Auf dem S3
// gibt es kein Fenster, keine Soundkarte, keine Tastatur und keinen Debugger,
// deshalb sind die meisten Methoden wirkungslos (no-op). Sie muessen aber alle
// vorhanden sein: osd_interface ist rein virtuell, und running_machine ruft
// z.B. in jedem Frame osd().update() und osd().input_update() auf.
//
// Was hier absichtlich NICHT passiert:
//   * keine Grafikausgabe (docs/09: "keine Grafikausgabe"; das Layout des
//     Treibers wird nicht gezeichnet)
//   * keine Soundausgabe (das 1-Bit-DAC von mm4 wird spaeter ausgewertet,
//     siehe docs/12, Abschnitt Audio)
//   * keine Tastatureingabe ueber OSD (die Matrix wird direkt angekoppelt)
//
// Alles, was der Kern *zwingend* braucht, damit er linkt und laeuft, ist
// vorhanden; alles andere ist bewusst leer und im Protokoll vermerkt.
// ---------------------------------------------------------------------------

#include "s3_osd.h"

#include "emu.h"        // running_machine, device_t, ui_manager
#include "emuopts.h"
#include "render.h"
#include "../../../../integration/chessengine/include/s3_uci_console.h"
#include "../../../src/bauerino_retro_lcd.h"
#include "../../../src/retro_test_ui.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"

#include <cstdio>
extern "C" void s3_m6502_profile_report(int64_t elapsed) __attribute__((weak));

//--------------------------------------------------------------------------
// allgemeine Haken
//--------------------------------------------------------------------------

void s3_osd::init(running_machine &machine)
{
	m_machine = &machine;
	m_rate_wall = 0;
	m_profile_wall = esp_timer_get_time();
	m_profile_uci = m_profile_display = m_profile_yield = 0;
	m_profile_frames = 0;
	m_uci_started = false;
	// A real target is also required with no visible output: render_manager's
	// config_save() dereferences its UI target when a machine exits. This only
	// establishes the core's bookkeeping; update() does not draw its primitives.
	auto target = machine.render().target_alloc();
	target->set_bounds(480, 480);
	std::printf("s3_osd: init (Treiber %s)\n", machine.system().name);
}

void s3_osd::set_verbose(bool print_verbose)
{
	m_verbose = print_verbose;
}

void s3_osd::update(bool skip_redraw)
{
	const int64_t profile_enter = esp_timer_get_time();
	int64_t profile_uci_end = profile_enter;
	// OSD init precedes ioport/video setup; only open the VM once all devices
	// and their port fields have started and the core enters its running phase.
	if (m_machine && m_machine->phase() == machine_phase::RUNNING) {
		if (!m_uci_started) {
			m_uci_started = true;
			if (!mame_s3_uci_open(*m_machine)) m_machine->schedule_exit();
		}
		mame_s3_uci_poll();
		profile_uci_end = esp_timer_get_time();
		uint8_t digits[4];
		for (int i=0;i<4;++i) { char name[12]; std::snprintf(name,sizeof(name),"digit%d",i); digits[i]=m_machine->output().get_value(name); }
		bauerino_lcd_digits(digits);
		bauerino_lcd_flush();
	}
	const int64_t profile_display_end = esp_timer_get_time();
	m_profile_uci += profile_uci_end - profile_enter;
	m_profile_display += profile_display_end - profile_uci_end;
	// Actual emulated-time / wall-time measurement. A ROM idle loop is not
	// a search benchmark; report phase and raw segment values for the log.
	const int64_t wall = esp_timer_get_time();
	if (m_machine) {
		const double emulated = m_machine->time().as_double();
		if (!m_rate_wall) { m_rate_wall = wall; m_rate_emu = emulated; }
		else if (wall - m_rate_wall >= 5000000) {
			const double seconds = double(wall - m_rate_wall) / 1000000.0;
			retro_ui_rate((emulated-m_rate_emu)/seconds*100.0);
			std::printf("[MAME RATE] emu=%.3fs real=%.3fs speed=%.1f%% heap=%u psram=%u\n",
				emulated - m_rate_emu, seconds, (emulated - m_rate_emu) / seconds * 100.0,
				unsigned(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)),
				unsigned(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
			m_rate_wall = wall; m_rate_emu = emulated;
		}
	}
	// Einmal pro Frame aufgerufen. Kein Fenster -> nichts zu zeichnen.
	//
	// WICHTIG: hier wird der Kontrollfluss an den FreeRTOS-Scheduler
	// zurueckgegeben. MAMEs Emulationsschleife laeuft sonst ohne Pause durch,
	// der Leerlauf-Task verhungert und der Task-Watchdog
	// (CONFIG_ESP_TASK_WDT_TIMEOUT_S) schlaegt zu.
	const int64_t profile_yield_start = esp_timer_get_time();
	vTaskDelay(1);
	const int64_t profile_end = esp_timer_get_time();
	m_profile_yield += profile_end - profile_yield_start;
	++m_profile_frames;
	if (profile_end - m_profile_wall >= 5000000) {
		const double elapsed = double(profile_end - m_profile_wall);
		if (s3_m6502_profile_report) s3_m6502_profile_report(profile_end - m_profile_wall);
		// Wall time, not sampled CPU time: mutex waits and preemption belong
		// to their measured section. Outside-OSD includes emulation, core video,
		// scheduler and other work; do not label it CPU execution time.
		std::printf("[MAME PROFILE] real=%.3fs frames=%u uci=%.1f%% display=%.1f%% yield=%.1f%% outside_osd=%.1f%%\n",
			elapsed / 1000000.0, m_profile_frames,
			100.0 * m_profile_uci / elapsed, 100.0 * m_profile_display / elapsed,
			100.0 * m_profile_yield / elapsed,
			100.0 * (elapsed - m_profile_uci - m_profile_display - m_profile_yield) / elapsed);
		m_profile_wall = esp_timer_get_time();
		m_profile_uci = m_profile_display = m_profile_yield = 0;
		m_profile_frames = 0;
	}
	(void)skip_redraw;
}

void s3_osd::input_update()
{
	// Ohne echte Eingabegeraete gibt es nichts zu pollen. Die Tastaturmatrix
	// von mm4 (KEY.0/KEY.1) wird spaeter ueber ioport-Ports gesetzt, nicht
	// ueber das OSD.
}

//--------------------------------------------------------------------------
// Debugger: auf dem S3 nicht vorhanden
//--------------------------------------------------------------------------

void s3_osd::init_debugger()
{
	// debug_flags ist 0 (kein Debugger) -> MAME ruft das nicht auf.
}

void s3_osd::wait_for_debugger(device_t &device, bool firststop)
{
	// kein Debugger
	(void)device;
	(void)firststop;
}

//--------------------------------------------------------------------------
// Audio
//--------------------------------------------------------------------------

void s3_osd::update_audio_stream(const int16_t *buffer, int samples_this_frame)
{
	// Aufgerufen, wenn MAME einen Block fertiger Samples liefert. Die Ausgabe
	// ueber I2S/DAC des S3 ist ein eigener Schritt; hier wird nichts getan,
	// aber die Daten sind definiert "verworfen".
	(void)buffer;
	(void)samples_this_frame;
}

void s3_osd::set_mastervolume(int attenuation)
{
	(void)attenuation;
}

bool s3_osd::no_sound()
{
	// Sagt dem Kern, dass keine Tonausgabe erfolgt. Damit waehlt MAME einen
	// Pfad ohne Sample-Ausgabe (der DAC laeuft trotzdem, damit das Timing der
	// Emulation stimmt).
	return true;
}

//--------------------------------------------------------------------------
// Eingabe: keine OSD-Geraete
//--------------------------------------------------------------------------

void s3_osd::customize_input_type_list(std::vector<input_type_entry> &typelist)
{
	// Keine zusaetzlichen oder geaenderten Eingabetypen.
	(void)typelist;
}

//--------------------------------------------------------------------------
// Video / Menue
//--------------------------------------------------------------------------

void s3_osd::add_audio_to_recording(const int16_t *buffer, int samples_this_frame)
{
	(void)buffer;
	(void)samples_this_frame;
}

std::vector<ui::menu_item> s3_osd::get_slider_list()
{
	// Leere Liste: keine OSD-Regler (Lautstaerke, Bildhelligkeit, ...).
	return std::vector<ui::menu_item>();
}

//--------------------------------------------------------------------------
// Schriftarten: keine
//--------------------------------------------------------------------------

osd_font::ptr s3_osd::font_alloc()
{
	// nullptr = es gibt keine Schrift. render_font prueft das und faellt
	// sauber zurueck (der Kern braucht Schriften nur fuer die UI).
	return nullptr;
}

bool s3_osd::get_font_families(std::string const &font_path, std::vector<std::pair<std::string, std::string>> &result)
{
	(void)font_path;
	result.clear();
	return false;
}

//--------------------------------------------------------------------------
// Kommandozeile / MIDI: keine
//--------------------------------------------------------------------------

bool s3_osd::execute_command(const char *command)
{
	// Keine Kommandos auf dem S3 ausfuehrbar.
	(void)command;
	return false;
}

osd_midi_device *s3_osd::create_midi_device()
{
	// nullptr = kein MIDI-Geraet vorhanden.
	return nullptr;
}
