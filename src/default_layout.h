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
Size=106,255
Collapsed=0
DockId=0x00000008,0

[Window][Palette]
Pos=0,276
Size=106,444
Collapsed=0
DockId=0x00000009,0

[Window][Canvas]
Pos=108,19
Size=913,539
Collapsed=0
DockId=0x00000002,0

[Window][Preview]
Pos=1023,19
Size=257,701
Collapsed=0
DockId=0x00000005,0

[Window][Timeline]
Pos=108,560
Size=913,160
Collapsed=0
DockId=0x00000003,0

[Docking][Data]
DockSpace       ID=0x08BD597D Window=0x1BBC0F80 Pos=0,19 Size=1280,701 Split=X
  DockNode      ID=0x00000006 Parent=0x08BD597D SizeRef=106,536 Split=Y Selected=0xED61EBF5
    DockNode    ID=0x00000008 Parent=0x00000006 SizeRef=106,195 Selected=0x18A5FDB9
    DockNode    ID=0x00000009 Parent=0x00000006 SizeRef=106,339 Selected=0xED61EBF5
  DockNode      ID=0x00000007 Parent=0x08BD597D SizeRef=368,536 Split=X
    DockNode    ID=0x00000004 Parent=0x00000007 SizeRef=913,536 Split=Y Selected=0x5EE3988C
      DockNode  ID=0x00000002 Parent=0x00000004 SizeRef=256,539 CentralNode=1 Selected=0x5EE3988C
      DockNode  ID=0x00000003 Parent=0x00000004 SizeRef=256,160 Selected=0x4F89F0DC
    DockNode    ID=0x00000005 Parent=0x00000007 SizeRef=257,536 Selected=0xE41466B5
)ini";
