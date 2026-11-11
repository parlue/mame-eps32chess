"""Use MAME's stream-rate API for discarded MM IV/V DAC audio only."""
from pathlib import Path
import re


def prepare(source, destination):
    original = source.read_text(encoding='utf-8')
    text = original
    anchor = '\tvoid create_custom(running_machine &machine) override\n\t{'
    assert text.count(anchor) == 1
    registration = '''
        m_quiet_machine = &machine;
        machine.add_notifier(MACHINE_NOTIFY_RESET,
            machine_notify_delegate(&s3_machine_manager::quiet_dac, this));'''
    text = text.replace(anchor, anchor + registration)
    marker = 'private:\n\tstd::unique_ptr<ui_manager> m_headless_ui;'
    assert text.count(marker) == 1
    method = '''    void quiet_dac() {
        auto &machine = *m_quiet_machine;
        const auto &options = machine.options();
        if (std::strcmp(machine.system().name, "mm4") && std::strcmp(machine.system().name, "mm5")) return;
        if (!machine.osd().no_sound() || options.wav_write()[0] || options.avi_write()[0]) {
            std::printf("[MAME QUIET DAC] original rate retained for audio/recording\\n");
            return;
        }
        device_t *dac = machine.root_device().subdevice("dac");
        auto *sound = dynamic_cast<device_sound_interface *>(dac);
        int output = -1;
        auto *stream = sound ? sound->output_to_stream_output(0, output) : nullptr;
        if (!dac || dac->type() != DAC_1BIT || !stream || output != 0 || machine.sample_rate() != 11025)
            throw emu_fatalerror("Quiet DAC profile: unexpected device/stream/output rate");
        const int old_rate = stream->sample_rate();
        if (old_rate != 192000 && old_rate != 11025)
            throw emu_fatalerror("Quiet DAC profile: unexpected original stream rate");
        stream->set_sample_rate(machine.sample_rate());
        // MAME applies pending rates through its normal sound-update accounting.
        // No CPU/device clock, DAC code/write callback, or timer is changed.
        std::printf("[MAME QUIET DAC] original=%d requested=%d cpu_clock=%u timer_attoseconds=%lld\\n",
            old_rate, stream->sample_rate(), unsigned(machine.root_device().subdevice("maincpu")->clock()),
            (long long)machine.sound().update_attoseconds());
    }

'''
    text = text.replace(marker, method + marker + '\n    running_machine *m_quiet_machine = nullptr;')
    restored = text.replace(registration, '', 1).replace(method, '', 1).replace('\n    running_machine *m_quiet_machine = nullptr;', '', 1)
    assert restored == original, 'Only the optional reset callback may be added'
    # Preserve the source's own relative includes after relocating this overlay.
    def include(match):
        path = source.parent / match[1]
        return '#include "' + path.resolve().as_posix() + '"' if path.is_file() else match[0]
    text = re.sub(r'#include "([^"\n]+)"', include, text)
    # Put the DAC declaration after emu.h; its upstream header is unchanged.
    marker = '#include "emu.h"'
    assert text.count(marker) == 1
    text = text.replace(marker, marker + '\n#include "sound/dac.h"')
    if not destination.exists() or destination.read_text(encoding='utf-8') != text:
        destination.write_text(text, encoding='utf-8', newline='\n')


if __name__ == '__main__':
    import argparse
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('source', type=Path)
    p.add_argument('destination', type=Path)
    a = p.parse_args()
    prepare(a.source, a.destination)
