ENTRY(_start)

SECTIONS {
	PROVIDE(__ipl_start = LDR_LOAD_ADDR);
	. = __ipl_start;
	.text : {
		*(.text._start);
		*(._boot_cfg);
		*(._ipl_version);
		*(._octopus);
		*(.text*);
	}
	.data : {
		*(.data*);
		*(.rodata*);
		*(._payload_00);
		*(._payload_01);
	}
	__ldr_end = .;
	. = ALIGN(0x10);
	__ipl_end = .;
}
