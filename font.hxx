static const int fontw = 128, fonth = 96;
static const vec2f fontwh(128.000000, 96.000000);
static const int fontcol = 16;
static const int charw = 8;
static const int charh = 16;
static const vec2f charwh(8.000000,16.000000);
static const u32 fontdata[] = {
0x14140800, 0x080e0008, 0x00140808, 0x00000000, 0x14140800, 0x0811023e, 0x00081004, 0x00000000, 
0x14140800, 0x08114509, 0x08141004, 0x40000000, 0x7f000800, 0x00112209, 0x08001004, 0x20000000, 
0x14000800, 0x000e1009, 0x08001004, 0x10000000, 0x14000800, 0x0005083e, 0x7f001004, 0x08007f00, 
0x14000800, 0x00090448, 0x08001004, 0x04000000, 0x7f000800, 0x00512248, 0x08001004, 0x02000000, 
0x14000000, 0x00215148, 0x08001004, 0x01000000, 0x14000000, 0x005e203e, 0x00001004, 0x000c000c, 
0x14000800, 0x00000008, 0x00000808, 0x000c0008, 0x00000000, 0x00000000, 0x00000000, 0x00000004, 
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
0x3f3f083e, 0x3f3e7f41, 0x00003e3e, 0x3e000000, 0x40400c41, 0x40010141, 0x00004141, 0x41020020, 
0x40400a41, 0x40010141, 0x00004141, 0x41040010, 0x40400861, 0x40010141, 0x00004141, 0x40087f08, 
0x40400851, 0x40010141, 0x0c0c4141, 0x40100004, 0x3e3e0849, 0x403f3e7e, 0x0c0c7e3e, 0x30200002, 
0x40010845, 0x40414040, 0x00004041, 0x08100004, 0x40010843, 0x40414040, 0x00004041, 0x08087f08, 
0x40010841, 0x40414040, 0x00004041, 0x00040010, 0x40010841, 0x40414040, 0x0c0c4041, 0x00020020, 
0x3f7e7f3e, 0x403e3f40, 0x080c3e3e, 0x08000000, 0x00000000, 0x00000000, 0x04000000, 0x00000000, 
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
0x3e3f3e3e, 0x3e7f7f3f, 0x417e3e41, 0x3e414101, 0x41414141, 0x41010141, 0x21100841, 0x41436301, 
0x01414141, 0x01010141, 0x11100841, 0x41455501, 0x01414141, 0x01010141, 0x09100841, 0x41494901, 
0x01414159, 0x01010141, 0x05100841, 0x41514101, 0x013f7f25, 0x793f3f41, 0x0310087f, 0x41614101, 
0x01414125, 0x41010141, 0x05100841, 0x41414101, 0x01414139, 0x41010141, 0x09100841, 0x41414101, 
0x01414101, 0x41010141, 0x11110841, 0x41414101, 0x41414101, 0x41010141, 0x21110841, 0x41414101, 
0x3e3f417e, 0x3e017f3f, 0x410e3e41, 0x3e41417f, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
0x3e3f3e3f, 0x4141417f, 0x187f4141, 0x00080c00, 0x41414141, 0x41414108, 0x08402241, 0x00140800, 
0x01414141, 0x41414108, 0x08401441, 0x00220801, 0x01414141, 0x41414108, 0x08200822, 0x00000802, 
0x01414141, 0x41414108, 0x08100814, 0x00000804, 0x3e3f413f, 0x41414108, 0x08080808, 0x00000808, 
0x40054101, 0x41414108, 0x08040814, 0x00000810, 0x40094101, 0x49414108, 0x08020822, 0x00000820, 
0x40114901, 0x49224108, 0x08010841, 0x00000840, 0x41215101, 0x49144108, 0x08010841, 0x00000800, 
0x3e413e01, 0x36083e08, 0x187f0841, 0x7f000c00, 0x00004000, 0x00000000, 0x00000000, 0x00000000, 
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
0x00010004, 0x00300040, 0x01000001, 0x0000000c, 0x00010008, 0x00480040, 0x01000001, 0x00000008, 
0x00010010, 0x00080040, 0x01100801, 0x00000008, 0x00010000, 0x00080040, 0x01000001, 0x00000008, 
0x00010000, 0x00080040, 0x01000001, 0x00000008, 0x3e3f3e00, 0x3e3e3e7e, 0x111c0e3f, 0x3e3e3608, 
0x01414000, 0x41084141, 0x09100841, 0x41414908, 0x01417e00, 0x41084141, 0x05100841, 0x41414908, 
0x01414100, 0x41083f41, 0x0b100841, 0x41414908, 0x01414100, 0x41080141, 0x11100841, 0x41414908, 
0x7e3f7e00, 0x7e087e7e, 0x61103e41, 0x3e414130, 0x00000000, 0x40000000, 0x00100000, 0x00000000, 
0x00000000, 0x40000000, 0x00100000, 0x00000000, 0x00000000, 0x40000000, 0x00100000, 0x00000000, 
0x00000000, 0x3e000000, 0x000e0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
0x00000000, 0x00000008, 0x10000000, 0x55000408, 0x00000000, 0x00000008, 0x08000000, 0x00000808, 
0x00000000, 0x00000008, 0x08000000, 0x41000808, 0x00000000, 0x00000008, 0x08000000, 0x00000808, 
0x00000000, 0x00000008, 0x08000000, 0x41060808, 0x7e3e7e3f, 0x4141413e, 0x041e4163, 0x00491000, 
0x01414141, 0x49414108, 0x08104114, 0x41300808, 0x3e014141, 0x49414108, 0x08084108, 0x00000808, 
0x40014141, 0x49224108, 0x08044108, 0x41000808, 0x40014141, 0x49144108, 0x08024114, 0x00000808, 
0x3f017e3f, 0x36083e08, 0x103e7e63, 0x55000408, 0x00004001, 0x00000000, 0x00004000, 0x00000000, 
0x00004001, 0x00000000, 0x00004000, 0x00000000, 0x00004001, 0x00000000, 0x00004000, 0x00000000, 
0x00004001, 0x00000000, 0x00003e00, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 
};
