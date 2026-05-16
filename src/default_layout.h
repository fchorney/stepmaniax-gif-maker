#pragma once
/// Default ImGui window layout (embedded ini for first-launch docking arrangement).

// Default ImGui window layout (docked arrangement).
// Written to imgui.ini on first launch when no config exists.
inline const char *kDefaultImGuiIni = R"ini(
[Window][WindowOverViewport_11111111]
Pos=0,19
Size=1425,881
Collapsed=0

[Window][Debug##Default]
Pos=60,60
Size=400,400
Collapsed=0

[Window][Tools]
Pos=0,19
Size=261,252
Collapsed=0
DockId=0x00000008,0

[Window][Palette]
Pos=0,273
Size=261,627
Collapsed=0
DockId=0x00000009,0

[Window][Canvas]
Pos=263,19
Size=860,740
Collapsed=0
DockId=0x00000002,0

[Window][Preview]
Pos=1125,19
Size=300,372
Collapsed=0
DockId=0x00000001,0

[Window][Timeline]
Pos=263,761
Size=860,139
Collapsed=0
DockId=0x00000003,0

[Window][Settings]
Pos=433,102
Size=278,564
Collapsed=0

[Window][Export Result]
Pos=541,324
Size=198,71
Collapsed=0

[Window][History]
Pos=1125,393
Size=300,507
Collapsed=0
DockId=0x0000000A,0

[Window][Export Warning]
Pos=282,280
Size=360,263
Collapsed=0

[Window][New Animation]
Pos=439,370
Size=402,125
Collapsed=0

[Window][Unsaved Changes]
Pos=495,322
Size=290,75
Collapsed=0

[Window][Save Warning]
Pos=460,228
Size=360,263
Collapsed=0

[Window][Save Result]
Pos=551,324
Size=177,71
Collapsed=0

[Window][Import Result]
Pos=316,324
Size=268,71
Collapsed=0

[Docking][Data]
DockSpace       ID=0x08BD597D Window=0x1BBC0F80 Pos=0,19 Size=1425,881 Split=X
  DockNode      ID=0x00000006 Parent=0x08BD597D SizeRef=261,536 Split=Y Selected=0xED61EBF5
    DockNode    ID=0x00000008 Parent=0x00000006 SizeRef=106,252 Selected=0x18A5FDB9
    DockNode    ID=0x00000009 Parent=0x00000006 SizeRef=106,627 Selected=0xED61EBF5
  DockNode      ID=0x00000007 Parent=0x08BD597D SizeRef=1162,536 Split=X
    DockNode    ID=0x00000004 Parent=0x00000007 SizeRef=860,536 Split=Y Selected=0x5EE3988C
      DockNode  ID=0x00000002 Parent=0x00000004 SizeRef=256,740 CentralNode=1 Selected=0x5EE3988C
      DockNode  ID=0x00000003 Parent=0x00000004 SizeRef=256,139 Selected=0x4F89F0DC
    DockNode    ID=0x00000005 Parent=0x00000007 SizeRef=300,536 Split=Y Selected=0xE41466B5
      DockNode  ID=0x00000001 Parent=0x00000005 SizeRef=397,372 Selected=0xE41466B5
      DockNode  ID=0x0000000A Parent=0x00000005 SizeRef=397,507 Selected=0x344D3274
)ini";
