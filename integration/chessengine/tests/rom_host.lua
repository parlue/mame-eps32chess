-- Execute the ported cooperative bridge against a REAL original-0.220 ROM.
-- PC test only; device state is supplied by MAME, not by a mock fixture.
io.stdout:setvbuf("no")
local root=assert(os.getenv("RETRO_TEST_MODULE"))
local function source(name)
    local f=assert(io.open(root .. "/generated/" .. name .. ".lua","rb"))
    local s=f:read("*a");f:close();return s
end
local bootstrap=assert(load(source("bootstrap")))()
local make=assert(load(source("bridge")))()
local profiles={mm4=source("mm4"),mm5=source("mm5")}
local machine=manager:machine()
local queue={"uci","isready"}
local reads=0
local finished=false
local started=emu.time()
local test_moves=os.getenv("RETRO_TEST_MOVES") or "e2e4"
local test_level=os.getenv("RETRO_TEST_LEVEL") or "le 1"
local test_limit=tonumber(os.getenv("RETRO_TEST_LIMIT")) or 120
local native={driver=machine:system().name,description=machine:system().description}
native.receive=function()return table.remove(queue,1)end
native.interface_source=function(name)return profiles[name]end
native.has_field=function(tag,mask)
    local p=machine:ioport().ports[tag];return p and p:field(mask)~=nil
end
native.field=function(tag,mask,value)machine:ioport().ports[tag]:field(mask):set_value(value)end
native.output=function(name)return machine:outputs():get_value(name)end
native.reset=function()machine:soft_reset()end
native.video_get=function(key)return machine:video()[key]end
native.video_set=function(key,value)machine:video()[key]=value end
native.exit=function()machine:exit()end
native.log=function(message)print("[RETRO LOG] " .. message)end
native.budget=function(co)debug.sethook(co,function()error("instruction budget")end,"",1000000)end
native.emit=function(line)
    print("[RETRO TEST] " .. line)
    if line=="readyok" then
        reads=reads+1
        if reads==1 then
            queue={"setoption name Level value " .. test_level,"ucinewgame","isready"}
        elseif reads==2 then
            queue={"position startpos moves " .. test_moves,"isready"}
        elseif reads==3 then
            print("[RETRO POSITION READY] " .. test_moves)
            queue={"go"}
        end
    elseif line:match("^bestmove [a-h][1-8][a-h][1-8]") then
        print("[RETRO ROM PASS] " .. native.driver .. " " .. line)
        finished=true
        -- Allow the bridge to acknowledge the chosen move on the sensorboard.
        queue={"quit"}
    end
end
local bridge=make(bootstrap(native))
emu.register_frame(function()
    local ok,err=pcall(bridge.poll,emu.time())
    if not ok then print("[RETRO ROM FAIL] " .. tostring(err));machine:exit();return end
    if emu.time()-started>test_limit and not finished then
        print("[RETRO ROM FAIL] no bestmove within emulated test limit")
        machine:exit()
    end
end)
print("[RETRO TEST] real ROM, driver=" .. native.driver)
