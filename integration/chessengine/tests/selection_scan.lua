-- Compare the optimized model adapter to the original, with changing signals.
local root=assert(os.getenv('RETRO_TEST_MODULE'))
local original=assert(os.getenv('RETRO_ORIGINAL_INTERFACE'))
local function adapter(path,values,counts)
    local outputs={get_value=function(_,name)
        local kind=name:sub(1,5)=='digit' and 'digits' or 'leds'
        counts[kind]=counts[kind]+1
        return values[name] or 0
    end}
    local env=setmetatable({machine={outputs=function() return outputs end}}, {__index=_G})
    return assert(loadfile(path,'t',env))()
end
local values,old_count,new_count={},{digits=0,leds=0},{digits=0,leds=0}
local old=adapter(original,values,old_count)
local new=adapter(root..'/generated/mm4.lua',values,new_count)
local xcode={0x77,0x7c,0x39,0x5e,0x79,0x71,0x3d,0x76}
local ycode={0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f}
for scenario=0,511 do
    -- Valid coordinate pairs, invalid displays, bit 7, and LED transitions.
    values.digit0=xcode[scenario%8+1] | ((scenario%2)*0x80)
    values.digit1=ycode[(scenario//8)%8+1]
    values.digit2=scenario%3==0 and 0 or xcode[(scenario//16)%8+1]
    values.digit3=scenario%3==0 and 0 or ycode[(scenario//64)%8+1] | 0x80
    for i=0,63 do values['led'..i]=(i==scenario%64 or (scenario%5==0 and i%7==0)) and 1 or 0 end
    if scenario%17==0 then
        for i=0,3 do values['digit'..i]=0 end
        for i=0,63 do values['led'..i]=0 end
    end
    new.prepare_selection()
    for y=1,8 do for x=1,8 do
        assert(old.is_selected(x,y)==new.is_selected(x,y),'selection mismatch')
    end end
end
assert(old_count.digits==512*256 and new_count.digits==512*4)
assert(old_count.leds==new_count.leds)
print('[SELECTION PASS] 512 changing snapshots; 32768 identical square results; digit reads 256 -> 4 per scan; LED reads identical')
manager:machine():exit()
