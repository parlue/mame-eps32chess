// Native bring-up transport. The product UI will exchange the same UCI lines
// through a bounded mailbox; it must never call the VM from the UI task.
#include "retro_mame_machine.h"
#include "s3_uci_console.h"
#include "../../../esp32s3/src/bauerino_retro_lcd.h"
#include "../../../esp32s3/src/retro_test_ui.h"
#include <cstdio>
#include <memory>
#include <fcntl.h>
#include <unistd.h>

namespace {
std::unique_ptr<retro::MameMachine> adapter;
std::unique_ptr<retro::Runtime> runtime;
running_machine *machine_ptr = nullptr;
std::string input;
bool overflow = false;
}

bool mame_s3_uci_open(running_machine &machine)
{
    mame_s3_uci_close();
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    adapter = std::make_unique<retro::MameMachine>(machine);
    runtime = std::make_unique<retro::Runtime>(*adapter);
    if (!runtime->open()) {
        std::printf("[UCI ERROR] %s\n", runtime->error().c_str());
        mame_s3_uci_close();
        return false;
    }
    machine_ptr = &machine;
    retro_ui_open(machine.system().name);
    std::printf("[UCI READY] %s; Lua heap=%u; USB console\n", machine.system().name,
                unsigned(runtime->memoryUsed()));
    return true;
}

void mame_s3_uci_poll()
{
    if (!runtime || !machine_ptr) return;
    retro_ui_poll();
    char command[896];
    while(retro_ui_command(command,sizeof(command))) {
        std::printf("[UI SEND] %s\n",command);
        if(!runtime->send(command))std::printf("[UCI ERROR] UI command rejected\n");
    }
    // Bound each frame's input work, including malformed/unending input.
#ifndef BAUERINO_MAME_FRONTEND
    char buffer[256];
    const ssize_t count = ::read(STDIN_FILENO, buffer, sizeof(buffer));
    for (ssize_t i=0; i<count; ++i) {
        const char c=buffer[i];
        if (c=='\r' || c=='\n') {
            if (overflow) std::printf("info string error input line too long\n");
            else if (input=="screenshot") bauerino_lcd_capture();
            else if (input.rfind("tap ",0)==0) {
                int x,y;if(std::sscanf(input.c_str(),"tap %d %d",&x,&y)==2)retro_ui_tap(x,y);
            } else if (!input.empty()) {
                retro_ui_observe(input.c_str());
                if(!runtime->send(input))std::printf("info string error UCI command rejected\n");
            }
            input.clear(); overflow=false;
        } else if (!overflow) {
            if (input.size() < retro::Runtime::LineLimit) input.push_back(c);
            else { input.clear(); overflow=true; }
        }
    }
#endif
    runtime->poll(machine_ptr->time().as_double());
    std::string reply;
    while (runtime->read(reply)) { retro_ui_reply(reply.c_str()); bauerino_lcd_uci(reply.c_str()); std::printf("%s\n",reply.c_str()); }
    if (!runtime->error().empty()) {
        std::printf("[UCI ERROR] %s\n",runtime->error().c_str());
        machine_ptr->schedule_exit();
        mame_s3_uci_close();
    }
}

void mame_s3_uci_close()
{
    bool active=machine_ptr!=nullptr;
    runtime.reset(); // Releases held keys while the adapter/machine still live.
    adapter.reset();
    machine_ptr=nullptr;
    input.clear(); overflow=false;
    if(active)retro_ui_closed();
}
