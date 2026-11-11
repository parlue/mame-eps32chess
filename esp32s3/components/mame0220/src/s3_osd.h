// ---------------------------------------------------------------------------
// s3_osd.h - OSD-Ersatzschicht fuer ESP32-S3 (Meilenstein 1b/3).
//
// MAME 0.220 verlangt von jeder Plattform eine Implementierung von
// osd_interface (src/osd/osdepend.h) und eine Reihe freier OSD-Funktionen
// (src/osd/osdcore.h). Auf dem S3 gibt es weder ein Fenster noch Sound,
// Tastatur oder Debugger-Ausgabe, deshalb sind fast alle Methoden hier
// wirkungslos - aber sie muessen existieren, sonst linkt der Kern nicht
// (running_machine haelt die Manager als Mitglieder).
//
// Wichtig: Dies ist KEINE Aenderung an MAME. Es ist die Plattformschicht, die
// MAME selbst fuer jede Zielplattform verlangt (windows/ und sdl/ sind die
// Vorlagen aus dem Originalbaum).
// ---------------------------------------------------------------------------

#pragma once

#include "osdepend.h"

class s3_osd : public osd_interface
{
public:
	s3_osd() = default;
	virtual ~s3_osd() = default;

	// allgemein
	void init(running_machine &machine) override;
	void update(bool skip_redraw) override;
	void input_update() override;
	void set_verbose(bool print_verbose) override;

	// Debugger (auf dem S3 nicht vorhanden)
	void init_debugger() override;
	void wait_for_debugger(device_t &device, bool firststop) override;

	// Audio (mm4 nutzt nur ein 1-Bit-DAC; Ausgabe folgt spaeter, siehe docs/12)
	void update_audio_stream(const int16_t *buffer, int samples_this_frame) override;
	void set_mastervolume(int attenuation) override;
	bool no_sound() override;

	// Eingabe (Tastaturmatrix wird spaeter direkt angekoppelt)
	void customize_input_type_list(std::vector<input_type_entry> &typelist) override;

	// Video (keine Grafikausgabe auf dem S3)
	void add_audio_to_recording(const int16_t *buffer, int samples_this_frame) override;
	std::vector<ui::menu_item> get_slider_list() override;

	// Schriftarten (keine)
	osd_font::ptr font_alloc() override;
	bool get_font_families(std::string const &font_path, std::vector<std::pair<std::string, std::string>> &result) override;

	// Kommandozeile / MIDI (keine)
	bool execute_command(const char *command) override;
	osd_midi_device *create_midi_device() override;

private:
	bool m_verbose = false;
	bool m_uci_started = false;
	running_machine *m_machine = nullptr;
	int64_t m_rate_wall = 0;
	double m_rate_emu = 0;
	int64_t m_profile_wall = 0;
	int64_t m_profile_uci = 0, m_profile_display = 0, m_profile_yield = 0;
	unsigned m_profile_frames = 0;
};
