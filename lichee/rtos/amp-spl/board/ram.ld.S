ENTRY(_start)

MEMORY
{
	ram(rwx) : ORIGIN = CONFIG_RAM_START_ADDRESS, LENGTH = CONFIG_RAM_LENGTH
}

SECTIONS
{
	. = ALIGN(4);
	.text :
	{
		*(.start)
		*(.text)
		*(.text*)
	} > ram

	. = ALIGN(4);
	.rodata :
	{
		*(.rodata)
		*(.rodata*)
	} > ram

	.data :
	{
		. = ALIGN(4);
		PROVIDE(__global_pointer$ = . + 0x800);
		*(.data)
		*(.data*)
		. = ALIGN(16);
		__stack_start = .;
		. += CONFIG_STACK_SIZE;
		__stack_end = .;
		. = ALIGN(4);
	} > ram

	.bss :
	{
		. = ALIGN(4);
		__bss_start = .;
		*(.bss)
		*(.bss*)
		__bss_end = ALIGN(0x8);
	} > ram
}
