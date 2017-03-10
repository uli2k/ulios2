OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)
SECTIONS
{
	.text 0x08000000: {*(.text)}
	.data : {*(.data)}
	.bss : {*(.bss)}
}
