ENTRY(_start)

SECTIONS {
	PROVIDE(__ipl_start = LDR_LOAD_ADDR);
	. = __ipl_start;
	.text : {
		*(.text._start);
		KEEP(*(._boot_cfg));
		KEEP(*(._ipl_version));
		KEEP(*(._octopus));
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
