interface = {}

interface.level = "le 1"
interface.cur_level = nil

function interface.setdigit(n)
	if     (n == 0) then send_input(":KEY1_6", 0x80, 0.5)
	elseif (n == 1) then send_input(":KEY2_3", 0x80, 0.5)
	elseif (n == 2) then send_input(":KEY2_5", 0x80, 0.5)
	elseif (n == 3) then send_input(":KEY2_6", 0x80, 0.5)
	elseif (n == 4) then send_input(":KEY2_7", 0x80, 0.5)
	elseif (n == 5) then send_input(":KEY2_0", 0x80, 0.5)
	elseif (n == 6) then send_input(":KEY2_1", 0x80, 0.5)
	elseif (n == 7) then send_input(":KEY2_2", 0x80, 0.5)
	elseif (n == 8) then send_input(":KEY2_4", 0x80, 0.5)
	elseif (n == 9) then send_input(":KEY1_7", 0x80, 0.5)
	elseif (n == 10) then send_input(":KEY1_6", 0x80, 0.5)
	end
end

function interface.setlevel()
	if (interface.cur_level == nil or interface.cur_level == interface.level) then
		return
	end
	interface.cur_level = interface.level
	send_input(":KEY1_4", 0x80, 1) -- LEV
	if (interface.level:sub(1,2) == "bl") then
		send_input(":KEY1_4", 0x80, 1) -- LEV
	end
	local n = tonumber(interface.level:sub(4))
	interface.setdigit(n)
	send_input(":KEY1_5", 0x80, 1) -- ENT
end

function interface.setup_machine()
	sb_reset_board(":board:board")
	emu.wait(1.0)
--	send_input(":KEY1_0", 0x80, 1) -- CL
--	emu.wait(1.0)

	interface.cur_level = "le 1"
	interface.setlevel()
end

function interface.set_pos(mode)
	if mode == 0 then
		send_input(":KEY1_1", 0x80, 0.5) -- POS
		for i=1,14 do
			send_input(":KEY1_5", 0x80, 0.5) -- ENT
		end
		return (":board:board")
	else
		for y=1,8 do
			for x=1,8 do
				local p=get_piece_id(x,9-y)
				if p~=0 then
					if p>0 then
						interface.setdigit(0) -- White
					else
						interface.setdigit(9) -- Black
					end
					interface.setdigit(7-math.abs(p))
					interface.setdigit(x)
					interface.setdigit(9-y)
					send_input(":KEY1_5", 0x80, 0.5) -- ENT
				end
			end
		end
		if mode == 1 then
			interface.setdigit(0) -- White
		else
			interface.setdigit(9) -- Black
		end
		send_input(":KEY1_5", 0x80, 0.5) -- ENT
		send_input(":KEY1_0", 0x80, 0.5) -- CL
	end
end

function interface.edit_game(on)
	if on then
		send_input(":KEY1_2", 0x80, 0.5) -- MEM
	else
		send_input(":KEY1_0", 0x80, 0.5) -- CL
	end
	emu.wait(0.5)
end

function interface.start_play(init)
	send_input(":KEY1_5", 0x80, 1) -- ENT
end

local selection_x = { 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x3d, 0x76 }
local selection_y = { 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f }
local selection_digits = {0, 0, 0, 0}
local selection_leds = {}
for i=0,63 do selection_leds[i] = "led" .. tostring(i) end

function interface.prepare_selection()
    local outputs = machine:outputs()
    for i=0,3 do selection_digits[i+1] = outputs:get_value("digit" .. tostring(i)) & 0x7f end
end

function interface.is_selected(x, y)
    return (selection_x[x] == selection_digits[1] and selection_y[y] == selection_digits[2])
        or (selection_x[x] == selection_digits[3] and selection_y[y] == selection_digits[4])
        or machine:outputs():get_value(selection_leds[8 * (y - 1) + (x - 1)]) ~= 0
end

function interface.select_piece(x, y, event)
	local d0 = machine:outputs():get_value("digit0") & 0x7f
	local d1 = machine:outputs():get_value("digit1") & 0x7f
	local d2 = machine:outputs():get_value("digit2") & 0x7f
	local d3 = machine:outputs():get_value("digit3") & 0x7f
	if ((d0 == 0x31 and d1 == 0x30 and d2 == 0x37 and d3 == 0x79)
	or  (d0 == 0x77 and d1 == 0x06 and d2 == 0x77 and d3 == 0x06)) then -- 'TIME' or 'A1A1'
		send_input(":KEY1_0", 0x80, 1) -- CL
	end
	sb_select_piece(":board:board", 1.0, x, y, event)
end

function interface.get_options()
	return { { "string", "Level", "le 1"}, }
end

function interface.set_option(name, value)
	if (name == "level") then
		local level = string.lower(value:match("^%s*(.-)%s*$"):gsub("%s%s+"," ")) -- trim
		if (string.match(level,"^[0-9]$")) then
			level = "le " .. level
		end
		if (string.match(level,"^le [0-9]$") or string.match(level,"^bl [1-8]$")) then
			interface.level = level
			interface.setlevel()
		end
	end
end

function interface.get_promotion(x, y)
	send_input(":KEY1_3", 0x80, 0.5)	-- INFO
	send_input(":KEY2_3", 0x80, 0.5)	-- 1

	local d3 = 0
	for i=0,5 do
		local d0 = machine:outputs():get_value("digit0") & 0x7f
		local d1 = machine:outputs():get_value("digit1") & 0x7f

		if (d0 == 0x73 and d1 == 0x50) then	-- display shows 'Pr'
			d3 = machine:outputs():get_value("digit3") & 0x7f
			break
		end
		send_input(":KEY1_6", 0x80, 0.5)	-- 0
	end

	send_input(":KEY1_3", 0x80, 0.5)	-- INFO

	if     (d3 == 0x5e) then	return "q"
	elseif (d3 == 0x31) then	return "r"
	elseif (d3 == 0x38) then	return "b"
	elseif (d3 == 0x6d) then	return "n"
	end

	return nil
end

function interface.promote(x, y, piece)
	sb_promote(":board:board", x, y, piece)
	if     (piece == "q") then	send_input(":KEY2_0", 0x80, 1)
	elseif (piece == "r") then	send_input(":KEY2_7", 0x80, 1)
	elseif (piece == "b") then	send_input(":KEY2_6", 0x80, 1)
	elseif (piece == "n") then	send_input(":KEY2_5", 0x80, 1)
	end
	if (piece == "q" or piece == "r" or piece == "b" or piece == "n") then
		send_input(":KEY1_5", 0x80, 1)
	end
end

return interface
