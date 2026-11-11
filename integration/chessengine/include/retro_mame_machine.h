#pragma once
// Include from the MAME component once the active core build has completed.
// This header uses the actual public 0.220 APIs, without sol2/frontend bindings.
#include "emu.h"
#include "video.h"
#include "retro_runtime.h"
namespace retro {
class MameMachine final : public Machine {
    running_machine& m;
public:
    explicit MameMachine(running_machine& machine):m(machine){}
    const char* driver() const override{return m.system().name;}
    const char* description() const override{return m.system().type.fullname();}
    bool hasField(const char* tag,unsigned mask) override {
        auto port=m.root_device().ioport(tag);return port && port->field(mask);
    }
    void field(const char* tag,unsigned mask,int value) override {
        auto port=m.root_device().ioport(tag);if(port)if(auto f=port->field(mask))f->set_value(value);
    }
    int output(const char* name) override{return m.output().get_value(name);}
    void reset() override{m.schedule_soft_reset();}
    void speed(bool throttle,double rate) override{m.video().set_throttled(throttle);m.video().set_throttle_rate(float(rate));}
    void exit() override{m.schedule_exit();}
};
}
