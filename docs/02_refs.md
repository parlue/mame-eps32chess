# 02 - Referenzanalyse: Plugin, API, PicoChess-Integration

Erzeugt: 2026-11-09T07:00:19Z

## chessengine-Plugin (sronco, Repo-Tip)

Pfad: `ref\mame-chessengine\chessengine`
Interface-Dateien: **185**

MM-IV/MM-V-Interface vorhanden? mm1=True mm2=True **mm4=True mm5=True**

Mephisto-artige Interfaces (44):

```
academy
alm16
alm32
berl16
berl32t8p
berlinp
lond16
lond32
lond32t
lond32t8
lyon16
lyon32
lyon32t
lyon32t8
mega4
mephisto
mephisto1x
mephisto2
mephisto2e
mephisto3
mephistoj
milano
mm1
mm2
mm4
mm5
mm5p
mmirage
modena
mondial
mondial2
montec
montec4le
mrisc
nshort
polgar
port16
port32
port32t
rebel5
van16
van32
van32t
van32t8
```

## Verwendete MAME-Lua-API (Top-Level, Haeufigkeit)

| Symbol | Treffer |
|---|---:|
| `emu` | 464 |
| `machine` | 71 |
| `manager` | 20 |
| `ioport` | 11 |

## `plugin.json`

```json
{
  "plugin": {
	"name": "chessengine",
	"description": "Chess UCI/XBoard Interface plugin",
	"version": "0.0.1",
	"author": "Sandro Ronco",
	"type": "plugin",
	"start": "false"
  }
}

```

## `init.lua`

```lua
-- license:BSD-3-Clause
-- copyright-holders:Sandro Ronco

local exports = {}
exports.name = "chessengine"
exports.version = "0.0.1"
exports.description = "Chess UCI/XBoard Interface plugin"
exports.license = "The BSD 3-Clause License"
exports.author = { name = "Sandro Ronco" }


local plugin_path = ""
local protocol = ""
local co = nil
local conth = nil
local interface = nil
local board = nil
local game_started = false
local sel_started = false
local piece_get = false
local my_color = "B"
local ply = "W"
local piece_from = nil
local piece_to = nil
local prev_move = nil
local scr = [[
	while true do
		_G.status = io.stdin:read("*line")
		yield()
	end
]]

local function describe_system()
	return manager.machine.system.description .. " (" .. emu.app_name() .. " " .. emu.app_version() .. ")"
end

local function board_reset()
	game_started = false
	piece_get = false
	sel_started = false
	my_color = "B"
	ply = "W"
	piece_from = nil
	piece_to = nil
	prev_move = nil

	board = {{ 3, 5, 4, 2, 1, 4, 5, 3 },
		{  6, 6, 6, 6, 6, 6, 6, 6 },
		{  0, 0, 0, 0, 0, 0, 0, 0 },
		{  0, 0, 0, 0, 0, 0, 0, 0 },
		{  0, 0, 0, 0, 0, 0, 0, 0 },
		{  0, 0, 0, 0, 0, 0, 0, 0 },
		{ -6,-6,-6,-6,-6,-6,-6,-6 },
		{ -3,-5,-4,-2,-1,-4,-5,-3 }}

	if interface.setup_machine then
		interface.setup_machine()
	end
end

local function move_to_pos(move)
	local rows_idx = { a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8 }
	local x = rows_idx[move:sub(1, 1)]
	local y = tonumber(move:sub(2, 2))
	return {x = x, y = y}
end

local function promote_pawn(pos, piece, promotion)
	local sign = 1
	if board[pos.y][pos.x] < 0 then
		sign = -1
	end

	if     (piece == "q") then	board[pos.y][pos.x] = board[pos.y][pos.x] - (4 * sign)
	elseif (piece == "r") then	board[pos.y][pos.x] = board[pos.y][pos.x] - (3 * sign)
	elseif (piece == "b") then	board[pos.y][pos.x] = board[pos.y][pos.x] - (2 * sign)
	elseif (piece == "n") then	board[pos.y][pos.x] = board[pos.y][pos.x] - (1 * sign)
	end

	if interface.promote then
		emu.wait(0.5)
		if promotion then
			interface.promote(pos.x, pos.y, string.lower(piece))
		else
			interface.promote(pos.x, pos.y, string.upper(piece))
		end
	end
end

local function send_input(tag, mask, seconds)
	manager.machine.ioport.ports[tag]:field(mask):set_value(1)
	emu.wait(seconds * 2 / 3)
	manager.machine.ioport.ports[tag]:field(mask):set_value(0)
	emu.wait(seconds * 1 / 3)
end

local function sb_set_ui(tag, mask, state)
	local field = manager.machine.ioport.ports[tag .. ":UI"]:field(mask)
	if (field ~= nil) then
		field:set_value(state)
		emu.wait(0.5)
	end
end

local function sb_press_square(tag, seconds, x, y)
	sb_set_ui(tag, 0x0001, 1)
	send_input(tag .. ":RANK." .. tostring(y), 1 << (x - 1), seconds)
	sb_set_ui(tag, 0x0001, 0)
end

local function sb_promote(tag, x, y, piece)
	local mask = 0
	if     (string.lower(piece) == 'q') then	mask = 0x10
	elseif (string.lower(piece) == 'r') then	mask = 0x08
	elseif (string.lower(piece) == 'b') then	mask = 0x04
	elseif (string.lower(piece) == 'n') then	mask = 0x02
	end

	if board[y][x] < 0 or (board[y][x] == 0 and y == 8) then
		mask = mask << 6
	end

	send_input(tag .. ":SPAWN", mask, 0.09)
	if (manager.machine.output:get_value("piece_ui0") ~= 0) then
		sb_set_ui(tag, 0x0002, 1)
		send_input(tag .. ":RANK." .. tostring(y), 1 << (x - 1), 0.09)
		sb_set_ui(tag, 0x0002, 0)
	end
end

local function sb_remove_piece(tag, x, y)
	sb_set_ui(tag, 0x0002, 1)
	send_input(tag .. ":RANK." .. tostring(y), 1 << (x - 1), 0.09)
	sb_set_ui(tag, 0x0002, 0)
	send_input(tag .. ":UI", 0x0008, 0.09)	-- SensorBoard REMOVE
end

local function sb_select_piece(tag, seconds, x, y, event)
	if (event ~= "capture") then
		send_input(tag .. ":RANK." .. tostring(y), 1 << (x - 1), seconds)
	end
	if (event == "en_passant") then
		send_input(tag .. ":UI", 0x0008, seconds)	-- SensorBoard REMOVE
	end
end

local function sb_move_piece(tag, x, y)
	sb_set_ui(tag, 0x0002, 1)
	sb_select_piece(tag, 0.09, x, y, "")
	sb_set_ui(tag, 0x0002, 0)
end

local function sb_reset_board(tag)
	send_input(tag .. ":UI", 0x0200, 0.09)	-- SensorBoard RESET
end

local function sb_rotate_board(tag)
	sb_set_ui(tag, 0x0002, 1)
	send_input(tag .. ":UI", 0x0200, 0.09)	-- SensorBoard RESET
	sb_set_ui(tag, 0x0002, 0)
end

local function get_piece_id(x, y)
	if (board ~= nil) then
		return board[y][x]
	end
	return 0
end

local function get_move_type(fx, fy, tx, ty)
	if (fy == 1 and fx == 5 and ty == 1 and (tx == 3 or tx == 7) and get_piece_id(fx, fy) == 1) or
	   (fy == 8 and fx == 5 and ty == 8 and (tx == 3 or tx == 7) and get_piece_id(fx, fy) == -1) then
		return "castling"

	elseif (fx ~= tx and fy == 5 and ty == 6 and get_piece_id(fx, fy) == 6 and get_piece_id(tx, ty) == 0) or
	       (fx ~= tx and fy == 4 and ty == 3 and get_piece_id(fx, fy) == -6 and get_piece_id(tx, ty) == 0) then
		return "en_passant"

	elseif (get_piece_id(fx, fy) == 6 and ty == 8) or (get_piece_id(fx, fy) == -6 and ty == 1) then
		if (get_piece_id(tx, ty) ~= 0) then
			return "capture_promotion"
		else
			return "promotion"
		end

	elseif (get_piece_id(tx, ty) ~= 0) then
		return "capture"
	end

	return nil
end

local function recv_cmd()
	if conth.yield then
		return conth.result
	end
	return nil
end

local function send_cmd(cmd)
	io.stdout:write(cmd .. "\n")
	io.stdout:flush()
end

local function send_move(move)
	prev_move = move
	if (protocol == "xboard") then
		send_cmd("move " .. move)
	elseif (protocol == "uci") then
		send_cmd("bestmove " .. move)
	end
end

local function make_move(move, reason, promotion)
	local from = move_to_pos(move:sub(1, 2))
	local to = move_to_pos(move:sub(3, 4))

	if interface.select_piece then
		if not piece_get then
			interface.select_piece(from.x, from.y, "get" .. reason)
			emu.wait(0.5)
		end
		if (move:len() >= 5) and interface.promote_special then
			interface.promote_special(move:sub(move:len())) -- some engines need at promotion the piece between from and to!
		end
		if board[to.y][to.x] ~= 0 then
			interface.select_piece(to.x, to.y, "capture")
		end

		interface.select_piece(to.x, to.y, "put" .. reason)
	end

	piece_get = false
	sel_started = false

	-- castling
	if     (board[from.y][from.x] ==  1 and move == "e1g1") then make_move("h1f1", "_castling", false)
	elseif (board[from.y][from.x] ==  1 and move == "e1c1") then make_move("a1d1", "_castling", false)
	elseif (board[from.y][from.x] == -1 and move == "e8g8") then make_move("h8f8", "_castling", false)
	elseif (board[from.y][from.x] == -1 and move == "e8c8") then make_move("a8d8", "_castling", false)
	else
		-- next ply
		if (ply == "W") then
			ply = "B"
		else
			ply = "W"
		end
	end

	-- en passant
	if board[to.y][to.x] == 0 and board[from.y][from.x] == -6 and from.y == 4 and to.y == 3 and from.x ~= to.x and board[to.y + 1][to.x] == 6 then
		if interface.select_piece  then
			interface.select_piece(to.x, to.y + 1, "en_passant")
			emu.wait(0.5)
		end
		board[to.y + 1][to.x] = 0
	elseif board[to.y][to.x] == 0 and board[from.y][from.x] == 6  and from.y == 5 and to.y == 6 and from.x ~= to.x and board[to.y - 1][to.x] == -6 then
		if interface.select_piece then
			interface.select_piece(to.x, to.y - 1, "en_passant")
			emu.wait(0.5)
		end
		board[to.y - 1][to.x] = 0
	end

	board[to.y][to.x] = board[from.y][from.x]
	board[from.y][from.x] = 0

	-- promotion
	if (move:len() >= 5) then
		promote_pawn(to, move:sub(move:len()), promotion)
	end
end

local function search_selected_piece()
	local active_fpos = 0
	local active_tpos = 0
	local board_sel = {}
	if (interface.is_selected) then
		for y=1,8 do
			for x=1,8 do
				board_sel[y*8 + x] = interface.is_selected(x, y)
				if board_sel[y*8 + x] and ((board[y][x] < 0 and ply == "B") or (board[y][x] > 0 and ply == "W")) then
					piece_from = {x = x, y = y}
					active_fpos = active_fpos + 1
				end
			end
		end
		if (piece_from ~= nil) then
			for y=1,8 do
				for x=1,8 do
					if board_sel[y*8 + x] and (board[y][x] == 0 or (board[y][x] < 0 and ply == "W") or (board[y][x] > 0 and ply == "B")) then
						piece_to = {x = x, y = y}
						active_tpos = active_tpos + 1
					end
				end
			end
		end
	end

	-- If there are more than 2 selections, something is wrong
	if active_tpos > 1 or active_fpos > 1 or (piece_from ~= nil and piece_to ~= nil and piece_from.x == piece_to.x and piece_from.y == piece_to.y) then
		piece_from = nil
		piece_to = nil
	end

	-- in some systems LEDs flash for a bit after the search is completed, wait for 1 second should allow thing to stabilize
	if (not sel_started and (piece_from or piece_to)) then
		sel_started = true
		emu.wait(1)
		piece_from = nil
		piece_to = nil
	end

	if not piece_get and piece_from ~= nil and piece_to == nil then
		piece_get = true
		if interface.select_piece then
			interface.select_piece(piece_from.x, piece_from.y, "get")
			emu.wait(0.5)
		end
	end

	if piece_to ~= nil and piece_from ~= nil then
		local rows = { "a", "b", "c", "d", "e", "f", "g", "h" }
		local move = rows[piece_from.x] .. tostring(piece_from.y)
		move = move .. rows[piece_to.x] .. tostring(piece_to.y)
		local need_promotion = (piece_to.y == 8 and board[piece_from.y][piece_from.x] == 6) or (piece_to.y == 1 and board[piece_from.y][piece_from.x] == -6)

		-- promotion
		if (need_promotion) then
			local new_type = "q"	-- default to Queen
			if interface.get_promotion then
				new_type = interface.get_promotion(piece_to.x, piece_to.y)
			end

			if (new_type ~= nil) then
				move = move .. new_type
				need_promotion = false
				send_move(move)
			end
		else
			send_move(move)
		end

		make_move(move, "", false)

		-- some machines show the promotion only after the pawn has been moved
		if (need_promotion) then
			local new_type = nil
			if interface.get_promotion then
				new_type = interface.get_promotion(piece_to.x, piece_to.y)
			end

			if (new_type == nil) then
				emu.print_verbose(manager.machine.system.name .. " Unable to determine the promotion")
				new_type = "q"	-- default to Queen
			end

			promote_pawn(piece_to, new_type, false)
			move = move .. new_type
			send_move(move)
		end
	end
end

local function send_options()
	local tag_default = ""
	local tag_min = " "
	local tag_max = " "

	if (protocol == "uci") then
		tag_default = "default "
		tag_min = " min "
		tag_max = " max "
	end

	for idx,opt in ipairs(interface.get_options()) do
		local opt_data = nil
		if     (#opt == 3 and opt[1] == "string") then    opt_data = tag_default .. tostring(opt[3])
		elseif (#opt == 2 and opt[1] == "button") then    opt_data = ''
		elseif (#opt == 5 and opt[1] == "spin")   then    opt_data = tag_default .. tostring(opt[3]) .. tag_min .. tostring(opt[4]) .. tag_max .. tostring(opt[5])
		elseif (#opt == 3 and opt[1] == "check")  then
			if protocol == "uci" then
				opt_data = tag_default .. tostring(opt[3]:gsub("1", "true"):gsub("0", "false"))
			elseif protocol == "xboard" then
				opt_data = tag_default .. tostring(opt[3])
			end
		elseif (#opt == 4 and opt[1] == "combo")  then
			if protocol == "uci" then
				opt_data = tag_default .. tostring(opt[3]) .. " var " .. tostring(opt[4]):gsub("\t", " var ")
			elseif protocol == "xboard" then
				opt_data = tostring(opt[4]):gsub("%f[%w_]" .. tostring(opt[3]) .. "%f[^%w_]", "*" .. tostring(opt[3])):gsub("\t", " /// ")
			end
		end
		if (opt_data ~= nil) then
			if protocol == "uci" then
				send_cmd('option name ' .. tostring(opt[2])  .. ' type ' .. tostring(opt[1]) .. ' ' .. opt_data)
			elseif protocol == "xboard" then
				send_cmd('feature option="' .. tostring(opt[2])  .. ' -' .. tostring(opt[1]) .. ' ' .. opt_data .. '"')
			end
		else
			emu.print_verbose("Invalid interface options '" .. tostring(opt[1]) .. " " .. tostring(opt[2]) .. "'")
		end
	end
end

local function set_option(name, value)
	if (name == nil or value == nil) then
		return
	end

	if (string.lower(name) == "speed") then
		if (tonumber(value) == 0) then	-- 0 = unlimited
			manager.machine.video.throttled = false
		else
			manager.machine.video.throttled = true
			manager.machine.video.throttle_rate = tonumber(value) / 100.0
		end
	elseif (interface.set_option) then
		interface.set_option(string.lower(name), value)
	end
end

local function execute_uci_command(cmd)
	if cmd:match("^position fen") ~= nil then
		local m = cmd:find("moves")
		if m ~= nil then
			cmd = "position startpos " .. cmd:sub(m)
		end
	end
	if cmd == "uci" then
		protocol = cmd
		send_cmd("id name " .. describe_system())
		send_cmd("option name Speed type spin default 100 min 0 max 10000")
		if interface.get_options then
			send_options()
		end
		send_cmd("uciok")
	elseif cmd == "isready" then
		send_cmd("readyok")
	elseif cmd == "ucinewgame" then
		if game_started == true then
			game_started = false
			manager.machine:soft_reset()
		end
		board_reset()
	elseif cmd == "quit" then
		manager.machine:exit()
	elseif cmd:match("^go") ~= nil then
		if board == nil then
			board_reset()
		end
		if game_started == false or my_color ~= ply then
			if interface.start_play then
				interface.start_play(not game_started)
			end
			my_color = ply
			game_started = true
			sel_started = false
		end
	elseif cmd:match("^setoption name ") ~= nil then
		local opt_name, opt_val = string.match(cmd:sub(16), '(.+) value (.+)')
		if     (string.lower(opt_val) == "true" ) then opt_val = "1"
		elseif (string.lower(opt_val) == "false") then opt_val = "0"
		end
		set_option(opt_name, opt_val)
	elseif cmd:match("^position startpos moves") ~= nil then
		if board == nil then
			board_reset()
		end
		game_started = true
		local last_move = ""
		for i in string.gmatch(cmd, "%S+") do
			last_move = i
		end
		if (last_move == prev_move) then
			my_color = ply
			sel_started = false
			if interface.start_play then
				interface.start_play(not game_started)
			end
		else
			make_move(last_move, "", true)
			piece_from = nil
			piece_to = nil
			sel_started = false
		end
	elseif cmd == "stop" then
		if game_started == true then
			if interface.stop_play then
				interface.stop_play()
			elseif interface.start_play then
				interface.start_play(not game_started)
			end
		end
	else
		emu.print_verbose("Unhandled UCI command '" .. cmd .. "'")
	end
end

local function execute_xboard_command(cmd)
	if cmd == "xboard" then
		protocol = cmd
	elseif cmd:match("^protover") then
		send_cmd("feature done=0")
		send_cmd("feature myname=\"" .. describe_system() .. "\" colors=0 usermove=1 sigint=0 sigterm=0")
		send_cmd('feature option="Speed -spin 100 0 10000"')
		if interface.get_options then
			send_options()
		end
		send_cmd("feature done=1")
	elseif cmd == "new" then
		if game_started == true then
			game_started = false
			manager.machine:soft_reset()
		end
		board_reset()
	elseif cmd == "go" then
		if (board == nil) then
			board_reset()
		end
		sel_started = false
		if game_started == false or my_color ~= ply then
			if interface.start_play then
				interface.start_play(not game_started)
			end
			game_started = true
			my_color = ply
		end
	elseif (cmd == "?") then
		if interface.stop_play then
			interface.stop_play()
		elseif interface.start_play then
			interface.start_play(not game_started)
		end
	elseif cmd == "quit" then
		manager.machine:exit()
	elseif cmd:match("^option ") ~= nil then
		local opt_name, opt_val = string.match(cmd:sub(8), '([^=]+)=([^=]+)')
		set_option(opt_name, opt_val)
	elseif cmd:match("^usermove ") ~= nil then
		if board == nil then
			board_reset()
		end
		game_started = true
		make_move(cmd:sub(10), "", true)
		piece_from = nil
		piece_to = nil
		sel_started = false
	else
		emu.print_verbose("Unhandled xboard command '" .. cmd .. "'")
	end
end

local function update()
	repeat
		local command = recv_cmd()
		if (command ~= nil) then
			if (command == "uci" or command == "xboard") then
				protocol = command
			end

			if protocol == "uci" then
				execute_uci_command(command)
			elseif protocol == "xboard" then
				execute_xboard_command(command)
			end

			conth:continue(conth.result)
			emu.wait(0.1)
		end
	until command == nil

	-- search for a new move
	if ply == my_color then
		search_selected_piece()
	end
end

local function load_interface(name)
	local env = { machine = manager.machine, send_input = send_input, get_piece_id = get_piece_id, get_move_type = get_move_type, emu = emu,
			output = manager.machine.output, ioport = manager.machine.ioport, load_interface = load_interface,
			sb_select_piece = sb_select_piece, sb_move_piece = sb_move_piece, sb_press_square = sb_press_square, sb_promote = sb_promote,
			sb_remove_piece = sb_remove_piece, sb_reset_board = sb_reset_board, sb_rotate_board = sb_rotate_board, sb_set_ui = sb_set_ui,
			pairs = pairs, ipairs = ipairs, tostring = tostring, tonumber = tonumber, string = string, math = math, print = _G.print }

	local func = loadfile(plugin_path .. "/interfaces/" .. name .. ".lua", "t", env)
	if func then
		return func()
	end
	return nil
end

function exports.set_folder(path)
	plugin_path = path
end

function exports.startplugin()
	conth = emu.thread()
	conth:start(scr)

	emu.register_periodic(
	function()
		if ((co == nil or coroutine.status(co) == "dead") and not manager.machine.paused) then
			co = coroutine.create(update)
			coroutine.resume(co)
		end
	end)

	emu.register_start(
	function()
		local system = manager.machine.system.name
		if interface == nil then
			interface = load_interface(system)
		end
		if interface == nil and manager.machine.system.parent ~= nil then
			interface = load_interface(manager.machine.system.parent)
		end

		if interface == nil then
			interface = {}
			emu.print_error("Error: missing interface for " .. system)
		end
	end)

	emu.register_stop(
	function()

	end)
end

return exports

```

## Beispiel-Interface `polgar.lua`

```lua
-- license:BSD-3-Clause

interface = {}

interface.level = "NORMAL 0:10"
interface.cur_level = nil
interface.levelnum = 1

local function setdigits(n,r,s)
	local ddram = emu.item(machine.devices[':display:hd44780'].items['0/m_ddram'])
	send_input(":KEY", 0x40, 0.25) -- ENT
	for i=1,n do
		if (n == 3 and i == 2) then
			r = r + 1
			s = s + 1
		end
		while (string.char(ddram:read(0x40+r+i-1)) ~= interface.level:sub(s+i-1,s+i-1)) do
			if     (i == 1) then send_input(":KEY", 0x01, 0.25) -- TRN
			elseif (i == 2) then send_input(":KEY", 0x02, 0.25) -- INFO
			elseif (i == 3) then send_input(":KEY", 0x04, 0.25) -- MEM
			elseif (i == 4) then send_input(":KEY", 0x08, 0.25) -- POS
			end
		end
	end
	send_input(":KEY", 0x80, 0.25) -- CL
end

function interface.setlevel()
	if (interface.cur_level == nil or interface.cur_level == interface.level or interface.levelnum == 0) then
		return
	end
	send_input(":KEY", 0x10, 0.5) -- LEV
	emu.wait(0.5)
	local ddram = emu.item(machine.devices[':display:hd44780'].items['0/m_ddram']):read_block(0x40, 0x10)
	local temp = string.upper(ddram:sub(1,5))
	local n = 0
	if     (temp == " NORM" or temp == "NORMA") then n = 1
	elseif (temp == "TURN " or temp == "TOUR ") then n = 2
	elseif (temp == "BLITZ") then n = 3
	elseif (temp == "RECHE" or string.match(temp,"PLY[%-%s]D")) then n = 4
	elseif (temp == "MATT " or temp == "MATE ") then n = 5
	elseif (string.match(temp,"ELO[%-%s]T")) then n = 6
	elseif (string.match(temp,"ELO[%-%s]A")) then n = 7
	elseif (string.match(temp,"-- %u%u")) then n = 8
	else
		send_input(":KEY", 0x80, 0.5) -- CL
		return
	end
	interface.cur_level = interface.level
	n = (interface.levelnum - n + 8) % 8
	for i=1,n do
		send_input(":KEY", 0x10, 0.25) -- LEV
	end
	local s = 0
	if (interface.levelnum == 1) then
		s = string.find(interface.level,"%d:[0-5]%d")
		setdigits(3,12,s)
	elseif (interface.levelnum == 2) then
		s = string.find(interface.level,"%d%d")
		setdigits(2,5,s)
		s = string.find(interface.level,"%d:[0-5]%d",s+1)
		setdigits(3,12,s)
	elseif (interface.levelnum == 3) then
		s = string.find(interface.level,"%d:[0-5]%d")
		setdigits(3,7,s)
		s = string.find(interface.level,"%d:[0-5]%d",s+1)
		setdigits(3,12,s)
	elseif (interface.levelnum == 4) then
		s = string.find(interface.level,"%d%d")
		setdigits(2,12,s)
	elseif (interface.levelnum == 5) then
		s = string.find(interface.level,"[1-8]")
		setdigits(1,8,s)
	elseif (interface.levelnum == 6) then
		s = string.find(interface.level,"%d%d%d%d")
		setdigits(4,12,s)
	elseif (interface.levelnum == 7) then
		s = string.find(interface.level,"%d%d%d%d")
		setdigits(4,12,s)
	elseif (interface.levelnum == 8) then
		-- nothing to do
	end
	send_input(":KEY", 0x80, 0.5) -- CL
end

function interface.setup_machine()
	sb_reset_board(":board:board")
	emu.wait(1.0)

	interface.cur_level = ""
	interface.setlevel()
end

function interface.start_play(init)
	send_input(":KEY", 0x40, 1)
end

function interface.is_selected(x, y)
	return output:get_indexed_value("led", (y - 1) * 8 + (x - 1)) ~= 0
end

function interface.select_piece(x, y, event)
	sb_select_piece(":board:board", 1, x, y, event)
end

function interface.get_options()
	return { { "string", "Level", "NORMAL 0:10"}, }
end

function interface.set_option(name, value)
	if (name == "level" and value ~= "") then
		local temp = string.upper(value)
		interface.levelnum = 0
		if     ((string.match(temp,"NORMAL%s+%d:[0-5]%d")) or (string.match(temp,"NORMAL%s+TIME%s+%d:[0-5]%d"))) then
			interface.levelnum = 1
		elseif ((string.match(temp,"TURN%s+%d%d%s+IN%s+%d:[0-5]%d")) or (string.match(temp,"TOUR%s+%d%d%s+IN%s+%d:[0-5]%d"))) then
			interface.levelnum = 2
		elseif (string.match(temp,"BLITZ%s+%d:[0-5]%d%s+%d:[0-5]%d")) then
			interface.levelnum = 3
		elseif ((string.match(temp,"RECHENTIEFE%s+%d%d?")) or (string.match(temp,"PLY[%-%s]DEPTH%s+%d%d?"))) then
			if (string.match(temp," %d%d") == nil) then
				local s = string.find(temp," %d")
				value = value:sub(1,s) .. "0" .. value:sub(s+1,s+1)
			end
			interface.levelnum = 4
		elseif ((string.match(temp,"MATT%s+IN%s+[1-8]")) or (string.match(temp,"MATE%s+IN%s+[1-8]"))) then
			interface.levelnum = 5
		elseif ((string.match(temp,"ELO[%-%s]TURNIER%s+%d%d%d%d")) or (string.match(temp,"ELO[%-%s]TOUR%s+%d%d%d%d"))) then
			interface.levelnum = 6
		elseif ((string.match(temp,"ELO[%-%s]AKTIV%s+%d%d%d%d")) or (string.match(temp,"ELO[%-%s]ACTIVE%s+%d%d%d%d"))) then
			interface.levelnum = 7
		elseif ((string.match(temp,"ANALYSE")) or (string.match(temp,"INFINITE")) or (string.match(temp,"UNBEGRENZT")) or (string.match(temp,"UNENDLICH"))) then
			interface.levelnum = 8
		end
		if (interface.levelnum ~= 0) then
			interface.level = value
			interface.setlevel()
		end
	end
end

function interface.get_promotion(x, y)
	-- HD44780 Display Data RAM
	local ddram = emu.item(machine.devices[':display:hd44780'].items['0/m_ddram']):read_block(0x00, 0x80)

	if     (ddram:sub(65,81):find('\x04') or ddram:sub(65,81):find('\x0c')) then return 'q'
	elseif (ddram:sub(65,81):find('\x03') or ddram:sub(65,81):find('\x0b')) then return 'r'
	elseif (ddram:sub(65,81):find('\x02') or ddram:sub(65,81):find('\x0a')) then return 'b'
	elseif (ddram:sub(65,81):find('\x01') or ddram:sub(65,81):find('\x09')) then return 'n'
	end

	return nil
end

function interface.promote(x, y, piece)
	sb_promote(":board:board", x, y, piece)
	emu.wait(1.0)
	if     (piece == "q") then send_input(":KEY", 0x10, 1)
	elseif (piece == "r") then send_input(":KEY", 0x08, 1)
	elseif (piece == "b") then send_input(":KEY", 0x02, 1)
	elseif (piece == "n") then send_input(":KEY", 0x04, 1)
	end
end

return interface

```

## MAME-4-PicoChess (ScallyBag)

Verzeichnisse (Auszug):

```
./ (3 Dateien)
armv7l-MAME-new/ (13 Dateien)
mame_emulation_new/ (9 Dateien)
mame_emulation_new\Arena/ (12 Dateien)
mame_emulation_new\Arena\Languages/ (1 Dateien)
mame_emulation_new\Arena11/ (11 Dateien)
mame_emulation_new\Arena11\Languages/ (1 Dateien)
mame_emulation_new\artwork/ (32 Dateien)
mame_emulation_new\artwork\chess/ (13 Dateien)
mame_emulation_new\cfg/ (262 Dateien)
mame_emulation_new\folders/ (18 Dateien)
mame_emulation_new\hash/ (16 Dateien)
mame_emulation_new\Hiarcs/ (3 Dateien)
mame_emulation_new\history/ (1 Dateien)
mame_emulation_new\ini/ (263 Dateien)
mame_emulation_new\nvram/ (0 Dateien)
mame_emulation_new\nvram\academy/ (1 Dateien)
mame_emulation_new\nvram\academyg/ (1 Dateien)
mame_emulation_new\nvram\academyga/ (1 Dateien)
mame_emulation_new\nvram\alm16/ (1 Dateien)
mame_emulation_new\nvram\alm32/ (1 Dateien)
mame_emulation_new\nvram\alm32_oc/ (1 Dateien)
mame_emulation_new\nvram\arb/ (1 Dateien)
mame_emulation_new\nvram\arbv2/ (1 Dateien)
mame_emulation_new\nvram\berl16/ (1 Dateien)
mame_emulation_new\nvram\berl16a/ (1 Dateien)
mame_emulation_new\nvram\berl16a_oc/ (1 Dateien)
mame_emulation_new\nvram\berl16l/ (1 Dateien)
mame_emulation_new\nvram\berl16l_oc/ (1 Dateien)
mame_emulation_new\nvram\berl16_oc/ (1 Dateien)
mame_emulation_new\nvram\berl32t8p/ (1 Dateien)
mame_emulation_new\nvram\berlinp/ (1 Dateien)
mame_emulation_new\nvram\berlinpl/ (1 Dateien)
mame_emulation_new\nvram\ccmk5/ (1 Dateien)
mame_emulation_new\nvram\ccmk6/ (1 Dateien)
mame_emulation_new\nvram\cfortea/ (1 Dateien)
mame_emulation_new\nvram\cforteb/ (1 Dateien)
mame_emulation_new\nvram\corona/ (1 Dateien)
mame_emulation_new\nvram\coronaa/ (1 Dateien)
mame_emulation_new\nvram\diablo68/ (1 Dateien)
```

Gefundene Versionsangaben:

| Datei | 0.2xx |
|---|---|
| `README.md` | 0.219 |
| `mame_emulation_new\Readme.txt` | 0.220 |
| `mame_emulation_new\ini\___empty.ini` | 0.299 |
| `mame_emulation_new\ini\academy.ini` | 0.299 |
| `mame_emulation_new\ini\academyg.ini` | 0.299 |
| `mame_emulation_new\ini\academyga.ini` | 0.299 |
| `mame_emulation_new\ini\alm16.ini` | 0.299 |
| `mame_emulation_new\ini\alm32.ini` | 0.299 |
| `mame_emulation_new\ini\alm32_oc.ini` | 0.299 |
| `mame_emulation_new\ini\amsterd.ini` | 0.299 |
| `mame_emulation_new\ini\amsterda.ini` | 0.299 |
| `mame_emulation_new\ini\arb.ini` | 0.299 |
| `mame_emulation_new\ini\arbv2.ini` | 0.299 |
| `mame_emulation_new\ini\berl16.ini` | 0.299 |
| `mame_emulation_new\ini\berl16_oc.ini` | 0.299 |
| `mame_emulation_new\ini\berl16a.ini` | 0.299 |
| `mame_emulation_new\ini\berl16a_oc.ini` | 0.299 |
| `mame_emulation_new\ini\berl16l.ini` | 0.299 |
| `mame_emulation_new\ini\berl16l_oc.ini` | 0.299 |
| `mame_emulation_new\ini\berl32t8p.ini` | 0.299 |
| `mame_emulation_new\ini\berlinp.ini` | 0.299 |
| `mame_emulation_new\ini\berlinpl.ini` | 0.299 |
| `mame_emulation_new\ini\boris.ini` | 0.299 |
| `mame_emulation_new\ini\borisa.ini` | 0.299 |
| `mame_emulation_new\ini\borisdpl.ini` | 0.299 |
| `mame_emulation_new\ini\bup.ini` | 0.299 |
| `mame_emulation_new\ini\bupa.ini` | 0.299 |
| `mame_emulation_new\ini\cc10.ini` | 0.299 |
| `mame_emulation_new\ini\cc10a.ini` | 0.299 |
| `mame_emulation_new\ini\cc3.ini` | 0.299 |
| `mame_emulation_new\ini\cc7.ini` | 0.299 |
| `mame_emulation_new\ini\cc7o.ini` | 0.299 |
| `mame_emulation_new\ini\ccdelta1.ini` | 0.299 |
| `mame_emulation_new\ini\ccdelta1x.ini` | 0.299 |
| `mame_emulation_new\ini\ccmk1.ini` | 0.299 |
| `mame_emulation_new\ini\ccmk2.ini` | 0.299 |
| `mame_emulation_new\ini\ccmk2a.ini` | 0.299 |
| `mame_emulation_new\ini\ccmk5.ini` | 0.299 |
| `mame_emulation_new\ini\ccmk6.ini` | 0.299 |
| `mame_emulation_new\ini\cfortea.ini` | 0.299 |
| `mame_emulation_new\ini\cforteb.ini` | 0.299 |
| `mame_emulation_new\ini\ch2001.ini` | 0.299 |
| `mame_emulation_new\ini\chessmst.ini` | 0.299 |
| `mame_emulation_new\ini\chessmsta.ini` | 0.299 |
| `mame_emulation_new\ini\chessmstdm.ini` | 0.299 |
| `mame_emulation_new\ini\chesster.ini` | 0.299 |
| `mame_emulation_new\ini\chesstera.ini` | 0.299 |
| `mame_emulation_new\ini\chesstrv.ini` | 0.299 |
| `mame_emulation_new\ini\chmate.ini` | 0.299 |
| `mame_emulation_new\ini\ckmaster.ini` | 0.299 |
| `mame_emulation_new\ini\cmpchess.ini` | 0.299 |
| `mame_emulation_new\ini\cncchess.ini` | 0.299 |
| `mame_emulation_new\ini\cncchess2.ini` | 0.299 |
| `mame_emulation_new\ini\concply.ini` | 0.299 |
| `mame_emulation_new\ini\concplyv.ini` | 0.299 |
| `mame_emulation_new\ini\concstd.ini` | 0.299 |
| `mame_emulation_new\ini\const.ini` | 0.299 |
| `mame_emulation_new\ini\const36.ini` | 0.299 |
| `mame_emulation_new\ini\const36a.ini` | 0.299 |
| `mame_emulation_new\ini\constq.ini` | 0.299 |
| `mame_emulation_new\ini\corona.ini` | 0.299 |
| `mame_emulation_new\ini\coronaa.ini` | 0.299 |
| `mame_emulation_new\ini\cp2000.ini` | 0.299 |
| `mame_emulation_new\ini\csc.ini` | 0.299 |
| `mame_emulation_new\ini\csce.ini` | 0.299 |
| `mame_emulation_new\ini\dallas16.ini` | 0.299 |
| `mame_emulation_new\ini\dallas16a.ini` | 0.299 |
| `mame_emulation_new\ini\dallas32.ini` | 0.299 |
| `mame_emulation_new\ini\debutm.ini` | 0.299 |
| `mame_emulation_new\ini\diablo68.ini` | 0.299 |
| `mame_emulation_new\ini\exchess.ini` | 0.299 |
| `mame_emulation_new\ini\fdes2000.ini` | 0.299 |
| `mame_emulation_new\ini\fdes2000d.ini` | 0.299 |
| `mame_emulation_new\ini\fdes2100.ini` | 0.299 |
| `mame_emulation_new\ini\fdes2100d.ini` | 0.299 |
| `mame_emulation_new\ini\fdes2265.ini` | 0.299 |
| `mame_emulation_new\ini\fdes2325.ini` | 0.299 |
| `mame_emulation_new\ini\feag.ini` | 0.299 |
| `mame_emulation_new\ini\feag2100.ini` | 0.299 |
| `mame_emulation_new\ini\feag2100a.ini` | 0.299 |
| `mame_emulation_new\ini\feagv10.ini` | 0.299 |
| `mame_emulation_new\ini\feagv11.ini` | 0.299 |
| `mame_emulation_new\ini\feagv2.ini` | 0.299 |
| `mame_emulation_new\ini\feagv3.ini` | 0.299 |
| `mame_emulation_new\ini\feagv5.ini` | 0.299 |
| `mame_emulation_new\ini\feagv7.ini` | 0.299 |
| `mame_emulation_new\ini\feagv7a.ini` | 0.299 |
| `mame_emulation_new\ini\feagv9.ini` | 0.299 |
| `mame_emulation_new\ini\feasbu.ini` | 0.299 |
| `mame_emulation_new\ini\feasgla.ini` | 0.299 |
| `mame_emulation_new\ini\feleg.ini` | 0.299 |
| `mame_emulation_new\ini\fepriv.ini` | 0.299 |
| `mame_emulation_new\ini\fex68k.ini` | 0.299 |
| `mame_emulation_new\ini\fex68ka.ini` | 0.299 |
| `mame_emulation_new\ini\fex68kb.ini` | 0.299 |
| `mame_emulation_new\ini\fex68km2.ini` | 0.299 |
| `mame_emulation_new\ini\fex68km2a.ini` | 0.299 |
| `mame_emulation_new\ini\fex68km3.ini` | 0.299 |
| `mame_emulation_new\ini\fex68km4.ini` | 0.299 |
| `mame_emulation_new\ini\fexcel.ini` | 0.299 |
| `mame_emulation_new\ini\fexcel12.ini` | 0.299 |
| `mame_emulation_new\ini\fexcel124.ini` | 0.299 |
| `mame_emulation_new\ini\fexcela.ini` | 0.299 |
| `mame_emulation_new\ini\fexceld.ini` | 0.299 |
| `mame_emulation_new\ini\fexcelp.ini` | 0.299 |
| `mame_emulation_new\ini\fexcelpb.ini` | 0.299 |
| `mame_emulation_new\ini\fexcelv.ini` | 0.299 |
| `mame_emulation_new\ini\fphantom.ini` | 0.299 |
| `mame_emulation_new\ini\fpres.ini` | 0.299 |
| `mame_emulation_new\ini\fpresbu.ini` | 0.299 |
| `mame_emulation_new\ini\fscc12.ini` | 0.299 |
| `mame_emulation_new\ini\fscc12b.ini` | 0.299 |
| `mame_emulation_new\ini\fscc6.ini` | 0.299 |
| `mame_emulation_new\ini\fscc8.ini` | 0.299 |
| `mame_emulation_new\ini\fscc9.ini` | 0.299 |
| `mame_emulation_new\ini\fscc9b.ini` | 0.299 |
| `mame_emulation_new\ini\fscc9c.ini` | 0.299 |
| `mame_emulation_new\ini\fscc9ps.ini` | 0.299 |
| `mame_emulation_new\ini\galileo.ini` | 0.299 |
| `mame_emulation_new\ini\gen32.ini` | 0.299 |
| `mame_emulation_new\ini\gen32_oc.ini` | 0.299 |
| `mame_emulation_new\ini\gen32a.ini` | 0.299 |
| `mame_emulation_new\ini\gen32a_oc.ini` | 0.299 |
| `mame_emulation_new\ini\gen32l.ini` | 0.299 |
| `mame_emulation_new\ini\gen32l_oc.ini` | 0.299 |
| `mame_emulation_new\ini\ggm.ini` | 0.299 |
| `mame_emulation_new\ini\glasgow.ini` | 0.299 |
| `mame_emulation_new\ini\granits.ini` | 0.299 |
| `mame_emulation_new\ini\intel02.ini` | 0.299 |
| `mame_emulation_new\ini\kishon.ini` | 0.299 |
| `mame_emulation_new\ini\kishona.ini` | 0.299 |
| `mame_emulation_new\ini\leonardo.ini` | 0.299 |
| `mame_emulation_new\ini\leonardoa.ini` | 0.299 |
| `mame_emulation_new\ini\lond16.ini` | 0.299 |
| `mame_emulation_new\ini\lond32.ini` | 0.299 |
| `mame_emulation_new\ini\lond32_oc.ini` | 0.299 |
| `mame_emulation_new\ini\lond32t.ini` | 0.299 |
| `mame_emulation_new\ini\lond32t8.ini` | 0.299 |
| `mame_emulation_new\ini\lond32t8x.ini` | 0.299 |
| `mame_emulation_new\ini\lyon16.ini` | 0.299 |
| `mame_emulation_new\ini\lyon32.ini` | 0.299 |
| `mame_emulation_new\ini\lyon32_oc.ini` | 0.299 |
| `mame_emulation_new\ini\lyon32t.ini` | 0.299 |
| `mame_emulation_new\ini\lyon32t8.ini` | 0.299 |
| `mame_emulation_new\ini\mchess.ini` | 0.299 |
| `mame_emulation_new\ini\megaiv.ini` | 0.299 |
| `mame_emulation_new\ini\megaiva.ini` | 0.299 |
| `mame_emulation_new\ini\mephisto.ini` | 0.299 |
| `mame_emulation_new\ini\mephisto1x.ini` | 0.299 |
| `mame_emulation_new\ini\mephisto2.ini` | 0.299 |
| `mame_emulation_new\ini\mephisto2a.ini` | 0.299 |
| `mame_emulation_new\ini\mephisto2e.ini` | 0.299 |
| `mame_emulation_new\ini\mephisto3.ini` | 0.299 |
| `mame_emulation_new\ini\mephisto3b.ini` | 0.299 |
| `mame_emulation_new\ini\mess.ini` | 0.299 |
| `mame_emulation_new\ini\milano.ini` | 0.299 |
| `mame_emulation_new\ini\milanoa.ini` | 0.299 |
| `mame_emulation_new\ini\mm1.ini` | 0.299 |
| `mame_emulation_new\ini\mm1b.ini` | 0.299 |
| `mame_emulation_new\ini\mm2.ini` | 0.299 |
| `mame_emulation_new\ini\mm2a.ini` | 0.299 |
| `mame_emulation_new\ini\mm2b.ini` | 0.299 |
| `mame_emulation_new\ini\mm2c.ini` | 0.299 |
| `mame_emulation_new\ini\mm2d.ini` | 0.299 |
| `mame_emulation_new\ini\mm4.ini` | 0.299 |
| `mame_emulation_new\ini\mm4a.ini` | 0.299 |
| `mame_emulation_new\ini\mm4b.ini` | 0.299 |
| `mame_emulation_new\ini\mm4tk.ini` | 0.299 |
| `mame_emulation_new\ini\mm5.ini` | 0.299 |
| `mame_emulation_new\ini\mm5a.ini` | 0.299 |
| `mame_emulation_new\ini\mm5p.ini` | 0.299 |
| `mame_emulation_new\ini\mm5ptk.ini` | 0.299 |
| `mame_emulation_new\ini\mm5tk.ini` | 0.299 |
| `mame_emulation_new\ini\mmirage.ini` | 0.299 |
| `mame_emulation_new\ini\modena.ini` | 0.299 |
| `mame_emulation_new\ini\modenaa.ini` | 0.299 |
| `mame_emulation_new\ini\modenab.ini` | 0.299 |
| `mame_emulation_new\ini\mondial.ini` | 0.299 |
| `mame_emulation_new\ini\mondial2.ini` | 0.299 |
| `mame_emulation_new\ini\mondl68k.ini` | 0.299 |
| `mame_emulation_new\ini\montec.ini` | 0.299 |
| `mame_emulation_new\ini\monteca.ini` | 0.299 |
| `mame_emulation_new\ini\monteciv.ini` | 0.299 |
| `mame_emulation_new\ini\montreux.ini` | 0.299 |
| `mame_emulation_new\ini\mrisc.ini` | 0.299 |
| `mame_emulation_new\ini\mrisc2.ini` | 0.299 |
| `mame_emulation_new\ini\nocto.ini` | 0.299 |
| `mame_emulation_new\ini\npresto.ini` | 0.299 |
| `mame_emulation_new\ini\nshort.ini` | 0.299 |
| `mame_emulation_new\ini\polgar.ini` | 0.299 |
| `mame_emulation_new\ini\polgar10.ini` | 0.299 |
| `mame_emulation_new\ini\polgar101.ini` | 0.299 |
| `mame_emulation_new\ini\port16.ini` | 0.299 |
| `mame_emulation_new\ini\port32.ini` | 0.299 |
| `mame_emulation_new\ini\port32_oc.ini` | 0.299 |
| `mame_emulation_new\ini\port32a.ini` | 0.299 |
| `mame_emulation_new\ini\port32a_oc.ini` | 0.299 |
| `mame_emulation_new\ini\port32t.ini` | 0.299 |
| `mame_emulation_new\ini\prodigy.ini` | 0.299 |
| `mame_emulation_new\ini\prschess.ini` | 0.299 |
| `mame_emulation_new\ini\prtchess.ini` | 0.299 |
| `mame_emulation_new\ini\rebel5.ini` | 0.299 |
| `mame_emulation_new\ini\rebel5a.ini` | 0.299 |
| `mame_emulation_new\ini\regence.ini` | 0.299 |
| `mame_emulation_new\ini\renaissa.ini` | 0.299 |
| `mame_emulation_new\ini\renaissaa.ini` | 0.299 |
| `mame_emulation_new\ini\risc2500.ini` | 0.299 |
| `mame_emulation_new\ini\risc2500a.ini` | 0.299 |
| `mame_emulation_new\ini\roma16.ini` | 0.299 |
| `mame_emulation_new\ini\roma16a.ini` | 0.299 |
| `mame_emulation_new\ini\roma32.ini` | 0.299 |
| `mame_emulation_new\ini\savant.ini` | 0.299 |
| `mame_emulation_new\ini\savant2.ini` | 0.299 |
| `mame_emulation_new\ini\sc2.ini` | 0.299 |
| `mame_emulation_new\ini\sc2a.ini` | 0.299 |
| `mame_emulation_new\ini\schess.ini` | 0.299 |
| `mame_emulation_new\ini\scorpio68.ini` | 0.299 |
| `mame_emulation_new\ini\scptchess.ini` | 0.299 |
| `mame_emulation_new\ini\sdtor.ini` | 0.299 |
| `mame_emulation_new\ini\sexperta.ini` | 0.299 |
| `mame_emulation_new\ini\sexperta1.ini` | 0.299 |
| `mame_emulation_new\ini\sexpertb.ini` | 0.299 |
| `mame_emulation_new\ini\sexpertc.ini` | 0.299 |
| `mame_emulation_new\ini\sexpertc1.ini` | 0.299 |
| `mame_emulation_new\ini\sfortea.ini` | 0.299 |
| `mame_emulation_new\ini\sfortea1.ini` | 0.299 |
| `mame_emulation_new\ini\sfortea2.ini` | 0.299 |
| `mame_emulation_new\ini\sforteb.ini` | 0.299 |
| `mame_emulation_new\ini\sfortec.ini` | 0.299 |
| `mame_emulation_new\ini\slc1.ini` | 0.299 |
| `mame_emulation_new\ini\slc1a.ini` | 0.299 |
| `mame_emulation_new\ini\smchess.ini` | 0.299 |
| `mame_emulation_new\ini\smondial.ini` | 0.299 |
| `mame_emulation_new\ini\smondial2.ini` | 0.299 |
| `mame_emulation_new\ini\smondialab.ini` | 0.299 |
| `mame_emulation_new\ini\smondialb.ini` | 0.299 |
| `mame_emulation_new\ini\sphinx40.ini` | 0.299 |
| `mame_emulation_new\ini\ssensor4.ini` | 0.299 |
| `mame_emulation_new\ini\sstar28k.ini` | 0.299 |
| `mame_emulation_new\ini\ssystem3.ini` | 0.299 |
| `mame_emulation_new\ini\stratos.ini` | 0.299 |
| `mame_emulation_new\ini\stratosa.ini` | 0.299 |
| `mame_emulation_new\ini\super9cc.ini` | 0.299 |
| `mame_emulation_new\ini\supercon.ini` | 0.299 |
| `mame_emulation_new\ini\tascr30.ini` | 0.299 |
| `mame_emulation_new\ini\tascr30a.ini` | 0.299 |
| `mame_emulation_new\ini\tascr30b.ini` | 0.299 |
| `mame_emulation_new\ini\tascr30g.ini` | 0.299 |
| `mame_emulation_new\ini\tascr40.ini` | 0.299 |
| `mame_emulation_new\ini\tascr40a.ini` | 0.299 |
| `mame_emulation_new\ini\tascr40b.ini` | 0.299 |
| `mame_emulation_new\ini\tascr40g.ini` | 0.299 |
| `mame_emulation_new\ini\tking.ini` | 0.299 |
| `mame_emulation_new\ini\tkinga.ini` | 0.299 |
| `mame_emulation_new\ini\tkingb.ini` | 0.299 |
| `mame_emulation_new\ini\tstar432.ini` | 0.299 |
| `mame_emulation_new\ini\uvc.ini` | 0.299 |
| `mame_emulation_new\ini\van16.ini` | 0.299 |
| `mame_emulation_new\ini\van32.ini` | 0.299 |
| `mame_emulation_new\ini\van32_oc.ini` | 0.299 |
| `mame_emulation_new\ini\van32t.ini` | 0.299 |
| `mame_emulation_new\ini\van32t8.ini` | 0.299 |
| `mame_emulation_new\ini\vcc.ini` | 0.299 |
| `mame_emulation_new\ini\vsc.ini` | 0.299 |
| `mame_emulation_new\mess.ini` | 0.299 |
| `mame_emulation_new\plugins\Readme.md` | 0.212 |

### Inhalt `mame_emulation_new/`

```
Arena
Arena11
Engines.lst
Hiarcs
MessChess.exe
Readme.txt
Shredder
WinBoard
artwork
cfg
folders
hash
history
ini
mess.ini
mess.zip
mess_g.ini
messinfo.dat
messui.ini
nvram
plugin.ini
plugins
roms
```

### Inhalt `armv7l-MAME-new/`

```
amsterd
amsterd.uci
dallas16
dallas16.uci
engines.ini
mm4
mm4.uci
mm5
mm5.uci
rebel5
rebel5.uci
roma32
roma32.uci
```

### `README.md` (Auszug)

```
# MAME-4-PicoChess
 
# Emulating Retro Chess Computers on PicoChess
 
This will only work on pre-existing PicoChess systems running on Raspbian Buster on a RPi 3, RPi 3b+ or RPi 4b.
 
Please be patient and bare in mind that:
* This is still a work in progress
* Certain functions like 'Alternate Move' & 'Take Back' do not work
* Loading some Engines, resetting the pieces to 'New Game' and closing or rebooting PicoChess can take a while
* You will need to match any time shown in the level settings by setting the same or similar times within the Time menu 
* Select 'No Book' in the Book menu, each Engine will then use it's own built in books file
* Not all Levels are included for each Engine, read the associated Chess Computer Manual or study the lua file for more information 
* Copyright prevents us from telling you where to find the complete ROM set, so please don't ask. However should you find the ROMs you can legally use those of any Chess Computer you own, but please don't post the address
  
We extend our thanks to the Author for all his hard work and for allowing us to use his lua files 
  
100% emulation speed for all Engines is guaranteed on the RPi 4b however only a few emulations run slower on earlier RPi's
  
To monitor the speed in your environment, navigate to /opt/picochess/engines/armv7l and type:
 
sudo ./engname  (i.e. sudo ./mm5)  
uci  
quit  
an average speed percentage will then be shown, e.g. :  
 
Al@PicoChess:/opt/picochess/engines/armv7l $ sudo ./mm5  
Error opening translation file English  
Warning: -video none doesn't make much sense without -seconds_to_run  
uci  
id name Mephisto MM V (set 1) (mess 0.219)  
option name Speed type spin default 100 min 0 max 10000  
option name Level type string default le 1  
uciok  
quit  
Average speed: 100.00% (8 seconds)  
 
# Install Instructions:

* Update your Buster system:  
sudo apt update && sudo apt upgrade
 
* Install the following Packages:  
sudo apt install git build-essential libsdl2-dev libsdl2-ttf-dev libfontconfig1-dev qt5-default zip unzip

* Install My MAME GitHub repository (important, it must be this folder to work):    
cd ~/  
sudo git clone https://github.com/ScallyBag/MAME-4-PicoChess  
 
* Change folders to the newly installed repository:  
cd ~/MAME-4-PicoChess
 
* Run my fully commented Bash file to install everything else:  
./AddMAME.sh  
 
* Reboot and enjoy
  
  
  
Al.
  
ScallyCoops@gmail.com
  

```

### `AddMAME.sh` (Auszug)

```
#!/bin/bash

# Distribute the files required for MAME-4-PicoChess
clear
echo "This will not work alongside my new Switchable Image as the folder names have changed"
echo "Hit CTRL C to cancel this Bash script"
sleep 7

# Closing PicoChess
clear
echo "Closing PicoChess"
sudo service picochess stop
wait

# Move mame_emulation to /opt/picochess/engines/
cd ~/MAME-4-PicoChess
echo ""
echo "Copying mame_emulation_new to /opt/picochess/engines & unzip mess.zip"
sleep 1
cp -r mame_emulation_new /opt/picochess/engines
cd /opt/picochess/engines/mame_emulation_new
unzip mess.zip
wait
rm mess.zip
./mess -cc

# Rename your existing armv7l to armv7lPICO and copy the armv7l-MAME-new file to /opt/picochess/engines
echo ""
echo "Moving your existing armv7l to armv7lPICO (unless it exists) & replacing with the new armv7l-MAME folder"
sleep 1
cd ~/MAME-4-PicoChess
cp -r armv7l-MAME-new /opt/picochess/engines
cd /opt/picochess/engines
mv -vn armv7l armv7lPICO
mv armv7l-MAME-new armv7l

echo ""
echo "Moving any existing mame_emulation to mame_emulation_old & replacing with the new mame_emulation folder"
sleep 1
mv mame_emulation mame_emulation_old
mv mame_emulation_new mame_emulation
 
echo ""
echo "You can revert to your existing engines by deleting armv7l & copying armv7lPICO to armv7l"
echo "This will still keep a copy of your original engine folder plus the armv7l-MAME engine folder"
echo "Now reboot and enjoy the 6 MAME engines"
echo  ""


```
