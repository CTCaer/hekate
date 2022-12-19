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

		/*
		 * To mitigate bad injectors/chainloaders,
		 * miss-align binary size to account for version info.
		 * !If version text is not appended, then use ". = ALIGN(4)"!
		 */
		 data_end_ua = .;
		 . = ((data_end_ua + 0x6 + 4 - 1) & ~(4 - 1)) - 6;
	}
	__ldr_end = .;
	. = ALIGN(0x10);
	__ipl_end = .;
}
