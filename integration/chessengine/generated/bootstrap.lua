-- Build only the MAME Lua surface used by the first MM IV / MM V interfaces.
-- Every device operation remains in the emulation task via the native host.
return function(native)
    local outputs = {get_value=function(_, name) return native.output(name) end,
        get_indexed_value=function(_, prefix, index) return native.output(prefix .. index) end}
    local ports = setmetatable({}, {__index=function(_, tag)
        return {field=function(_, mask)
            if not native.has_field(tag, mask) then return nil end
            return {set_value=function(_, value) native.field(tag, mask, value) end}
        end}
    end})
    local video = setmetatable({}, {
        __index=function(_, key) return native.video_get(key) end,
        __newindex=function(_, key, value) native.video_set(key, value) end})
    local machine = {
        system=function() return {name=native.driver,description=native.description,parent=""} end,
        ioport=function() return {ports=ports} end,
        outputs=function() return outputs end,
        video=function() return video end,
        soft_reset=function() native.reset() end,
        exit=function() native.exit() end,
        popmessage=function(_, message) native.log(message) end,
        logerror=function(_, message) native.log(message) end}
    return {
        manager={machine=function() return machine end},
        emu={print_error=function(message) error(message) end},
        receive=native.receive, emit=native.emit, interface_source=native.interface_source,
        budget=native.budget}
end
