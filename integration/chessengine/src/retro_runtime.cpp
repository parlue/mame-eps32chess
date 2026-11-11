#include "retro_runtime.h"
extern "C" {
#include <limits.h>
// ESP newlib's C++ limits.h can omit the C99 macros although the compiler
// supports 64-bit long long. Keep Lua's normal 64-bit integer ABI unchanged.
#if defined(ESP_PLATFORM) && !defined(LLONG_MAX)
#define LLONG_MAX __LONG_LONG_MAX__
#define LLONG_MIN (-__LONG_LONG_MAX__ - 1LL)
#endif
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#include "payload.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif
namespace retro {
enum Op {Receive,Emit,Source,HasField,Field,Output,Reset,VideoGet,VideoSet,Exit,Log,Budget};
void* Runtime::allocate(void* ud,void* ptr,size_t oldSize,size_t newSize){
    auto& self=*static_cast<Runtime*>(ud);
    if(!ptr)oldSize=0; // Lua passes a type tag for a new allocation.
    if(!newSize){std::free(ptr);self.memory-=oldSize;return nullptr;}
    if(newSize>MemoryLimit || self.memory-oldSize>MemoryLimit-newSize)return nullptr;
#ifdef ESP_PLATFORM
    void* next=heap_caps_realloc(ptr,newSize,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
#else
    void* next=std::realloc(ptr,newSize);
#endif
    if(next)self.memory=self.memory-oldSize+newSize;
    return next;
}
void Runtime::release(){
    for(const auto& key:held)machine.field(key.tag.c_str(),key.mask,0);
    held.clear();
}
void Runtime::fail(const std::string& message){failure=message.substr(0,1024);commands.clear();replies.clear();release();}
void Runtime::close(){
    release();if(state){lua_close(state);state=nullptr;}
    commands.clear();replies.clear();bridge=0;memory=0;exiting=false;
}
bool Runtime::loadFactory(const char* source,const char* name){
    return luaL_loadbuffer(state,source,std::strlen(source),name)==LUA_OK && lua_pcall(state,0,1,0)==LUA_OK;
}
bool Runtime::open(){
    close();failure.clear();throttled=true;rate=1;lastTime=0;
    state=lua_newstate(allocate,this);
    if(!state){failure="Cannot allocate retro Lua state in PSRAM";return false;}
    // Initialise under pcall as library loading may exceed the allocator budget.
    lua_pushcclosure(state,[](lua_State* L)->int {
        const luaL_Reg libraries[]={{"_G",luaopen_base},{LUA_COLIBNAME,luaopen_coroutine},
            {LUA_TABLIBNAME,luaopen_table},{LUA_STRLIBNAME,luaopen_string},
            {LUA_MATHLIBNAME,luaopen_math},{LUA_UTF8LIBNAME,luaopen_utf8},{nullptr,nullptr}};
        for(auto lib=libraries;lib->name;++lib){luaL_requiref(L,lib->name,lib->func,1);lua_pop(L,1);}return 0;
    },0);
    if(lua_pcall(state,0,0,0)!=LUA_OK){fail(lua_tostring(state,-1));close();return false;}
    lua_newtable(state);
    const char* functions[]={"receive","emit","interface_source","has_field","field","output","reset","video_get","video_set","exit","log","budget"};
    for(int i=0;i<=Budget;++i){lua_pushlightuserdata(state,this);lua_pushinteger(state,i);lua_pushcclosure(state,callback,2);lua_setfield(state,-2,functions[i]);}
    lua_pushstring(state,machine.driver());lua_setfield(state,-2,"driver");
    lua_pushstring(state,machine.description());lua_setfield(state,-2,"description");
    lua_setglobal(state,"native");
    if(!loadFactory(payload_bootstrap,"bootstrap")){fail(lua_tostring(state,-1));close();return false;}
    lua_getglobal(state,"native");
    if(lua_pcall(state,1,1,0)!=LUA_OK){fail(lua_tostring(state,-1));close();return false;}
    lua_setglobal(state,"host");
    if(!loadFactory(payload_bridge,"chessengine")){fail(lua_tostring(state,-1));close();return false;}
    lua_getglobal(state,"host");
    if(lua_pcall(state,1,1,0)!=LUA_OK){fail(lua_tostring(state,-1));close();return false;}
    bridge=luaL_ref(state,LUA_REGISTRYINDEX);
    // The full core leaves less free PSRAM than Lua's default 200% GC pause
    // assumes. Collect discarded loader chunks and start cycles before the
    // allocator reaches physical exhaustion (observed in the le 9 ROM test).
    lua_gc(state,LUA_GCSETPAUSE,110);
    lua_gc(state,LUA_GCCOLLECT,0);
    return true;
}
static bool safe(const std::string& s){return s.size()<=Runtime::LineLimit && s.find_first_of("\r\n")==std::string::npos && s.find('\0')==std::string::npos;}
bool Runtime::send(const std::string& line){
    if(!running() || !safe(line))return false;
    if(line=="quit"){commands.clear();commands.push_front(line);return true;}
    if(commands.size()>=QueueLimit)return false;
    size_t bytes=line.size();for(auto& c:commands)bytes+=c.size();if(bytes>8192)return false;
    // Reject invalid square tokens before they reach the Lua board array.
    if(line.rfind("position startpos moves ",0)==0){
        size_t start=24;
        if(line.size()==start)return false;
        while(start<line.size()){
            auto end=line.find(' ',start);if(end==std::string::npos)end=line.size();
            auto n=end-start;if(n!=4 && n!=5)return false;
            if(line[start]<'a'||line[start]>'h'||line[start+1]<'1'||line[start+1]>'8'||line[start+2]<'a'||line[start+2]>'h'||line[start+3]<'1'||line[start+3]>'8')return false;
            if(n==5 && !std::strchr("qrbn",line[start+4]))return false;
            start=end+1;
        }
    }
    // A malformed option without a name is not a valid button command either.
    if(line=="setoption name " || line=="setoption name")return false;
    commands.push_back(line);return true;
}
bool Runtime::read(std::string& line){if(replies.empty())return false;line=std::move(replies.front());replies.pop_front();return true;}
void Runtime::poll(double now){
    if(!running())return;
    if(!commands.empty() && commands.front()=="quit"){machine.exit();close();return;}
    if(!std::isfinite(now) || now<lastTime){fail("Retro emulation clock moved backwards");return;}
    lastTime=now;lua_rawgeti(state,LUA_REGISTRYINDEX,bridge);lua_getfield(state,-1,"poll");lua_pushnumber(state,now);
    if(lua_pcall(state,1,0,0)!=LUA_OK){fail(lua_tostring(state,-1));lua_settop(state,0);return;}
    lua_pop(state,1);
    // Small bounded incremental work per emulated frame, instead of repeated
    // emergency full collections triggered by failed PSRAM allocations.
    lua_gc(state,LUA_GCSTEP,8);
    if(exiting){close();exiting=true;}
}
int Runtime::callback(lua_State* L){
    auto& self=*static_cast<Runtime*>(lua_touserdata(L,lua_upvalueindex(1)));
    auto op=static_cast<Op>(lua_tointeger(L,lua_upvalueindex(2)));
    switch(op){
    case Receive:
        if(self.commands.empty())return 0;
        lua_pushlstring(L,self.commands.front().data(),self.commands.front().size());self.commands.pop_front();return 1;
    case Emit: {
        size_t n=0;const char* text=luaL_checklstring(L,1,&n);
        size_t bytes=n;for(auto& r:self.replies)bytes+=r.size();
        if(n>LineLimit || self.replies.size()>=QueueLimit || bytes>8192)return luaL_error(L,"Retro UCI output queue full");
        if(std::memchr(text,'\n',n)||std::memchr(text,'\r',n)||std::memchr(text,0,n))return luaL_error(L,"Invalid retro output line");
        self.replies.emplace_back(text,n);return 0;
    }
    case Source: {
        const char* name=luaL_checkstring(L,1);const char* text=std::strcmp(name,"mm4")==0?payload_mm4:std::strcmp(name,"mm5")==0?payload_mm5:nullptr;
        if(!text)return 0;
        lua_pushstring(L,text);return 1;
    }
    case HasField:lua_pushboolean(L,self.machine.hasField(luaL_checkstring(L,1),uint32_t(luaL_checkinteger(L,2))));return 1;
    case Field: {
        const char* tag=luaL_checkstring(L,1);auto mask=uint32_t(luaL_checkinteger(L,2));int value=int(luaL_checkinteger(L,3));
        if(!self.machine.hasField(tag,mask))return luaL_error(L,"Missing MAME input field %s",tag);
        auto match=std::find_if(self.held.begin(),self.held.end(),[&](const Held& h){return h.mask==mask && h.tag==tag;});
        if(value && match==self.held.end()){
            if(self.held.size()>=32)return luaL_error(L,"Too many held retro keys");
            self.held.push_back({tag,mask});
        }else if(!value && match!=self.held.end())self.held.erase(match);
        self.machine.field(tag,mask,value);return 0;
    }
    case Output:lua_pushinteger(L,self.machine.output(luaL_checkstring(L,1)));return 1;
    case Reset:self.release();self.machine.reset();return 0;
    case Exit:self.exiting=true;self.release();self.machine.exit();return 0;
    case VideoGet:
        if(std::strcmp(luaL_checkstring(L,1),"throttled")==0)lua_pushboolean(L,self.throttled);
        else lua_pushnumber(L,self.rate);
        return 1;
    case VideoSet:
        if(std::strcmp(luaL_checkstring(L,1),"throttled")==0)self.throttled=lua_toboolean(L,2)!=0;
        else {auto value=luaL_checknumber(L,2);if(!std::isfinite(value)||value<0||value>100)return luaL_error(L,"Invalid throttle rate");self.rate=value;}
        self.machine.speed(self.throttled,self.rate);return 0;
    case Log:return 0; // diagnostics are separate from UCI, never fake depth/eval
    case Budget: {
        auto thread=lua_tothread(L,1);if(!thread)return luaL_error(L,"Missing retro coroutine");
        lua_sethook(thread,[](lua_State* co,lua_Debug*){luaL_error(co,"Retro Lua instruction budget exceeded");},LUA_MASKCOUNT,1000000);return 0;
    }
    }
    return 0;
}
}
