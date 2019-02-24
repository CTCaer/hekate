ENTRY(_start)

SECTIONS {
	PROVIDE(__ipl_start = IPL_LOAD_ADDR);
	. = __ipl_start;
	.text : {
		*(.text._start);
		. = . + 36;
		*(.text*);
	}
	.data : {
		*(.data*);
		*(.rodata*);
	}
	. = ALIGN(0x10);
	__ipl_end = .;
	.bss : {
		__bss_start = .;
		*(COMMON)
		*(.bss*)
		__bss_end = .;
	}
}
