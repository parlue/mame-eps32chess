	.file	"xtensa_delegate_probe.cpp"
	.text
	.global	probe_direct
	.data
	.align	4
	.type	probe_direct, @object
	.size	probe_direct, 8
probe_direct:
	.word	_ZN16DelegateAbiProbe13direct_methodEv
	.word	0
	.global	probe_virtual
	.align	4
	.type	probe_virtual, @object
	.size	probe_virtual, 8
probe_virtual:
	.word	1
	.word	0
	.ident	"GCC: (crosstool-NG esp-13.2.0_20240530) 13.2.0"
