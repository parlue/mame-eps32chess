The generated Lua files are a local adaptation of the installed CB-Emu
MessChess plugin. Its `init.lua` identifies Sandro Ronco and Franz Huber as
authors, version 2024.1.1, and declares BSD-3-Clause. Preserve the original
package's attribution and terms when distributing an adaptation.

The Lua VM is taken unchanged from `ref/mame0220-src/3rdparty/lua`, version
5.3.4, copyright Lua.org / PUC-Rio. Its MIT license is in that source tree's
`doc/readme.html` and at the end of `src/lua.h`.

ROMs are kept separately in ignored `local-roms/`, never embedded in the Lua
payload or included in a firmware/archive by these tools. The ROM preparation
tool reads only the user's local CB-Emu installation.
