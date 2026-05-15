#pragma once

// Default ImGui window layout (docked arrangement).
// Written to imgui.ini on first launch when no config exists.
inline const char *kDefaultImGuiIni = R"ini(
[Window][WindowOverViewport_11111111]
Pos=0,19
Size=1280,701
Collapsed=0

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Tools]
Pos=0,19
Size=375,255
Collapsed=0
DockId=0x00000008,0

[Window][Palette]
Pos=0,276
Size=375,444
Collapsed=0
DockId=0x00000009,0

[Window][Canvas]
Pos=377,19
Size=576,555
Collapsed=0
DockId=0x00000002,0

[Window][Preview]
Pos=955,19
Size=325,423
Collapsed=0
DockId=0x00000001,0

[Window][Timeline]
Pos=377,576
Size=576,144
Collapsed=0
DockId=0x00000003,0

[Window][History]
Pos=955,444
Size=325,276
Collapsed=0
DockId=0x0000000A,0

[Docking][Data]
DockSpace       ID=0x08BD597D Window=0x1BBC0F80 Pos=0,19 Size=1280,701 Split=X
  DockNode      ID=0x00000006 Parent=0x08BD597D SizeRef=375,536 Split=Y Selected=0xED61EBF5
    DockNode    ID=0x00000008 Parent=0x00000006 SizeRef=106,195 Selected=0x18A5FDB9
    DockNode    ID=0x00000009 Parent=0x00000006 SizeRef=106,339 Selected=0xED61EBF5
  DockNode      ID=0x00000007 Parent=0x08BD597D SizeRef=2183,536 Split=X
    DockNode    ID=0x00000004 Parent=0x00000007 SizeRef=1856,536 Split=Y Selected=0x5EE3988C
      DockNode  ID=0x00000002 Parent=0x00000004 SizeRef=256,1195 CentralNode=1 Selected=0x5EE3988C
      DockNode  ID=0x00000003 Parent=0x00000004 SizeRef=256,144 Selected=0x4F89F0DC
    DockNode    ID=0x00000005 Parent=0x00000007 SizeRef=325,536 Split=Y Selected=0xE41466B5
      DockNode  ID=0x00000001 Parent=0x00000005 SizeRef=397,423 Selected=0xE41466B5
      DockNode  ID=0x0000000A Parent=0x00000005 SizeRef=397,276 Selected=0x344D3274
)ini";
