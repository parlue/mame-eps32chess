"""Diagnostic link-only override; do not change compiler flags or ROM code.

Use with PLATFORMIO_EXTRA_SCRIPTS=post:../tools/no_relax_link.py.
The original config.cpp object has distinct cfg_directory/default.cfg literals,
while the relaxed firmware loads default.cfg for both. Test without relaxation.
"""
Import("env")
env.Append(LINKFLAGS=["-Wl,--no-relax"])
