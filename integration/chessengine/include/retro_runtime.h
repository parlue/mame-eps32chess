#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>
struct lua_State;
namespace retro {
// All methods are called on the MAME task, never from the UI/BLE task.
class Machine {
public:
    virtual ~Machine()=default;
    virtual const char* driver() const=0;
    virtual const char* description() const=0;
    virtual bool hasField(const char* tag,unsigned mask)=0;
    virtual void field(const char* tag,unsigned mask,int value)=0;
    virtual int output(const char* name)=0;
    virtual void reset()=0;
    virtual void speed(bool throttled,double rate)=0;
    virtual void exit()=0;
};
// Memory-bounded Lua runtime, isolated from the active firmware build.
// Public methods belong to one task. Use a bounded FreeRTOS queue to exchange
// lines with the UI task; copy them here during the MAME periodic callback.
class Runtime {
public:
    static constexpr size_t MemoryLimit=768*1024, QueueLimit=32, LineLimit=4096;
    explicit Runtime(Machine& machine):machine(machine){}
    ~Runtime(){close();}
    Runtime(const Runtime&)=delete; Runtime& operator=(const Runtime&)=delete;
    bool open();
    bool send(const std::string& line);
    bool read(std::string& line);
    void poll(double emulatedSeconds);
    void close();
    const std::string& error() const{return failure;}
    bool running() const{return state!=nullptr && failure.empty() && !exiting;}
    size_t memoryUsed() const{return memory;}
private:
    Machine& machine;
    lua_State* state=nullptr;
    size_t memory=0;
    int bridge=0;
    bool exiting=false,throttled=true;
    double rate=1,lastTime=0;
    std::string failure;
    std::deque<std::string> commands,replies;
    struct Held {std::string tag;unsigned mask;};
    std::vector<Held> held;
    void fail(const std::string& message);
    void release();
    static void* allocate(void* ud,void* ptr,size_t oldSize,size_t newSize);
    static int callback(lua_State* L);
    bool loadFactory(const char* source,const char* name);
};
}
