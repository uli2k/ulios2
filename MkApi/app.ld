OUTPUT_FORMAT("binary")
ENTRY(_start)
SECTIONS
{
	.text 0x08000000: {*(.text)}
	.data : {*(.data)}
	.bss : {*(.bss)}
}
