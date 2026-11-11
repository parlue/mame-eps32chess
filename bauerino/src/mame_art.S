.section .rodata.bauerino_retro_base,"a",@progbits
.balign 4
.global _binary_retro_base_rgb565_start
_binary_retro_base_rgb565_start:
.incbin "../esp32s3/src/retro_base.rgb565"
.global _binary_retro_base_rgb565_end
_binary_retro_base_rgb565_end:
.balign 4
.global _binary_retro_pieces_start
_binary_retro_pieces_start:
.incbin "../esp32s3/src/retro_pieces.rgb565"
