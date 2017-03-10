OUTPUT_FORMAT("binary")
ENTRY(_start)
SECTIONS
{
	.text 0x08000000: {*(.text)}
	.data ALIGN(0x1000): {*(.data)}
	.bss : {*(.bss)}
}
