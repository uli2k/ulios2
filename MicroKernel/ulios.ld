OUTPUT_FORMAT("binary")
ENTRY(_start)
SECTIONS
{
	_gdt =	0;
	_idt =	0x0800;
	_kpdt =	0x1000;
	_pddt =	0x2000;
	_kmmt =	0x3000;
	_pmt =	0x6000;
	_kpt =	0x7000;

	_KnlValue =	0xF000;

	.text 0x10000: {*(.text)}
	.data : {*(.data)}
	.bss : {*(.bss)}

	_BaseSrv =	0x90080;
	_MemEnd =	0x900FC;
	_ards =		0x90100;

	_kdat =		0x00100000;
	_pdt =		0x00800000;
	_pt =		0x00C00000;
	_pt0 =		0x01000000;
	_pt2 =		0x01400000;
	_pg0 =		0x01800000;
}
