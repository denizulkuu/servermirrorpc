#ifndef CP437_MAP_H
#define CP437_MAP_H

#include <Arduino.h>

// ============================================================================
// CP437 (Code Page 437) to USB HID Keyboard Mapping
// ============================================================================
// Maps all 256 CP437 bytes to USB HID Usage ID keycodes.
// Includes modifier flags (Shift, AltGr) for uppercase/symbols.
// Extended CP437 characters (128-255) mapped to closest ASCII equivalents
// or sent via Alt+NumPad sequences where possible.

// USB HID Modifier Bitmask
#define HID_MOD_LEFTCTRL   0x01
#define HID_MOD_LEFTSHIFT  0x02
#define HID_MOD_LEFTALT    0x04
#define HID_MOD_LEFTGUI    0x08
#define HID_MOD_RIGHTCTRL  0x10
#define HID_MOD_RIGHTSHIFT 0x20
#define HID_MOD_RIGHTALT   0x40
#define HID_MOD_RIGHTGUI   0x80

// USB HID Keycodes (simplified subset)
#define HID_KEY_A        0x04
#define HID_KEY_B        0x05
#define HID_KEY_C        0x06
#define HID_KEY_D        0x07
#define HID_KEY_E        0x08
#define HID_KEY_F        0x09
#define HID_KEY_G        0x0A
#define HID_KEY_H        0x0B
#define HID_KEY_I        0x0C
#define HID_KEY_J        0x0D
#define HID_KEY_K        0x0E
#define HID_KEY_L        0x0F
#define HID_KEY_M        0x10
#define HID_KEY_N        0x11
#define HID_KEY_O        0x12
#define HID_KEY_P        0x13
#define HID_KEY_Q        0x14
#define HID_KEY_R        0x15
#define HID_KEY_S        0x16
#define HID_KEY_T        0x17
#define HID_KEY_U        0x18
#define HID_KEY_V        0x19
#define HID_KEY_W        0x1A
#define HID_KEY_X        0x1B
#define HID_KEY_Y        0x1C
#define HID_KEY_Z        0x1D
#define HID_KEY_1        0x1E
#define HID_KEY_2        0x1F
#define HID_KEY_3        0x20
#define HID_KEY_4        0x21
#define HID_KEY_5        0x22
#define HID_KEY_6        0x23
#define HID_KEY_7        0x24
#define HID_KEY_8        0x25
#define HID_KEY_9        0x26
#define HID_KEY_0        0x27
#define HID_KEY_ENTER    0x28
#define HID_KEY_ESCAPE   0x29
#define HID_KEY_BACKSPACE 0x2A
#define HID_KEY_TAB      0x2B
#define HID_KEY_SPACE    0x2C
#define HID_KEY_MINUS    0x2D
#define HID_KEY_EQUAL    0x2E
#define HID_KEY_LBRACKET 0x2F
#define HID_KEY_RBRACKET 0x30
#define HID_KEY_BACKSLASH 0x31
#define HID_KEY_SEMICOLON 0x33
#define HID_KEY_APOSTROPHE 0x34
#define HID_KEY_GRAVE    0x35
#define HID_KEY_COMMA    0x36
#define HID_KEY_PERIOD   0x37
#define HID_KEY_SLASH    0x38
#define HID_KEY_CAPSLOCK 0x39
#define HID_KEY_F1       0x3A
#define HID_KEY_F2       0x3B
#define HID_KEY_F3       0x3C
#define HID_KEY_F4       0x3D
#define HID_KEY_F5       0x3E
#define HID_KEY_F6       0x3F
#define HID_KEY_F7       0x40
#define HID_KEY_F8       0x41
#define HID_KEY_F9       0x42
#define HID_KEY_F10      0x43
#define HID_KEY_F11      0x44
#define HID_KEY_F12      0x45
#define HID_KEY_PRINTSCREEN 0x46
#define HID_KEY_SCROLLLOCK  0x47
#define HID_KEY_PAUSE    0x48
#define HID_KEY_INSERT   0x49
#define HID_KEY_HOME     0x4A
#define HID_KEY_PAGEUP   0x4B
#define HID_KEY_DELETE   0x4C
#define HID_KEY_END      0x4D
#define HID_KEY_PAGEDOWN 0x4E
#define HID_KEY_RIGHT    0x4F
#define HID_KEY_LEFT     0x50
#define HID_KEY_DOWN     0x51
#define HID_KEY_UP       0x52
#define HID_KEY_NUMLOCK  0x53
#define HID_KEY_KPSLASH  0x54
#define HID_KEY_KPASTERISK 0x55
#define HID_KEY_KPMINUS  0x56
#define HID_KEY_KPPLUS   0x57
#define HID_KEY_KPENTER  0x58
#define HID_KEY_KP1      0x59
#define HID_KEY_KP2      0x5A
#define HID_KEY_KP3      0x5B
#define HID_KEY_KP4      0x5C
#define HID_KEY_KP5      0x5D
#define HID_KEY_KP6      0x5E
#define HID_KEY_KP7      0x5F
#define HID_KEY_KP8      0x60
#define HID_KEY_KP9      0x61
#define HID_KEY_KP0      0x62
#define HID_KEY_KPDOT    0x63
#define HID_KEY_APP      0x65
#define HID_KEY_POWER    0x66

// Special sentinel: key not directly mappable (use Alt code sequence)
#define HID_KEY_NONE     0x00

// ============================================================================
// CP437 → HID Keycode Lookup Table (PROGMEM)
// ============================================================================
// Index: CP437 byte value (0-255)
// Value: HID keycode (0x00 = unmappable/special, handled separately)

const uint8_t cp437_hid_key[] PROGMEM = {
    // ---- Control Characters (0-31) ----
    HID_KEY_NONE,       // 0   NUL
    HID_KEY_NONE,       // 1   SOH
    HID_KEY_NONE,       // 2   STX
    HID_KEY_NONE,       // 3   ETX
    HID_KEY_NONE,       // 4   EOT
    HID_KEY_NONE,       // 5   ENQ
    HID_KEY_NONE,       // 6   ACK
    HID_KEY_NONE,       // 7   BEL
    HID_KEY_BACKSPACE,  // 8   BS
    HID_KEY_TAB,        // 9   HT
    HID_KEY_ENTER,      // 10  LF (→ Enter)
    HID_KEY_NONE,       // 11  VT
    HID_KEY_NONE,       // 12  FF
    HID_KEY_ENTER,      // 13  CR (→ Enter)
    HID_KEY_NONE,       // 14  SO
    HID_KEY_NONE,       // 15  SI
    HID_KEY_NONE,       // 16  DLE
    HID_KEY_NONE,       // 17  DC1
    HID_KEY_NONE,       // 18  DC2
    HID_KEY_NONE,       // 19  DC3
    HID_KEY_NONE,       // 20  DC4
    HID_KEY_NONE,       // 21  NAK
    HID_KEY_NONE,       // 22  SYN
    HID_KEY_NONE,       // 23  ETB
    HID_KEY_NONE,       // 24  CAN
    HID_KEY_NONE,       // 25  EM
    HID_KEY_NONE,       // 26  SUB
    HID_KEY_ESCAPE,     // 27  ESC
    HID_KEY_NONE,       // 28  FS
    HID_KEY_NONE,       // 29  GS
    HID_KEY_NONE,       // 30  RS
    HID_KEY_NONE,       // 31  US

    // ---- Printable ASCII (32-126) ----
    HID_KEY_SPACE,      // 32  ' '
    HID_KEY_1,          // 33  !   (Shift + 1)
    HID_KEY_APOSTROPHE, // 34  "   (Shift + ')
    HID_KEY_3,          // 35  #   (Shift + 3)
    HID_KEY_4,          // 36  $   (Shift + 4)
    HID_KEY_5,          // 37  %   (Shift + 5)
    HID_KEY_7,          // 38  &   (Shift + 7)
    HID_KEY_APOSTROPHE, // 39  '
    HID_KEY_9,          // 40  (   (Shift + 9)
    HID_KEY_0,          // 41  )   (Shift + 0)
    HID_KEY_8,          // 42  *   (Shift + 8)
    HID_KEY_EQUAL,      // 43  +   (Shift + =)
    HID_KEY_COMMA,      // 44  ,
    HID_KEY_MINUS,      // 45  -
    HID_KEY_PERIOD,     // 46  .
    HID_KEY_SLASH,      // 47  /
    HID_KEY_0,          // 48  0
    HID_KEY_1,          // 49  1
    HID_KEY_2,          // 50  2
    HID_KEY_3,          // 51  3
    HID_KEY_4,          // 52  4
    HID_KEY_5,          // 53  5
    HID_KEY_6,          // 54  6
    HID_KEY_7,          // 55  7
    HID_KEY_8,          // 56  8
    HID_KEY_9,          // 57  9
    HID_KEY_SEMICOLON,  // 58  :   (Shift + ;)
    HID_KEY_SEMICOLON,  // 59  ;
    HID_KEY_COMMA,      // 60  <   (Shift + ,)
    HID_KEY_EQUAL,      // 61  =
    HID_KEY_PERIOD,     // 62  >   (Shift + .)
    HID_KEY_SLASH,      // 63  ?   (Shift + /)
    HID_KEY_2,          // 64  @   (Shift + 2)
    HID_KEY_A,          // 65  A   (Shift + a)
    HID_KEY_B,          // 66  B   (Shift + b)
    HID_KEY_C,          // 67  C   (Shift + c)
    HID_KEY_D,          // 68  D   (Shift + d)
    HID_KEY_E,          // 69  E   (Shift + e)
    HID_KEY_F,          // 70  F   (Shift + f)
    HID_KEY_G,          // 71  G   (Shift + g)
    HID_KEY_H,          // 72  H   (Shift + h)
    HID_KEY_I,          // 73  I   (Shift + i)
    HID_KEY_J,          // 74  J   (Shift + j)
    HID_KEY_K,          // 75  K   (Shift + k)
    HID_KEY_L,          // 76  L   (Shift + l)
    HID_KEY_M,          // 77  M   (Shift + m)
    HID_KEY_N,          // 78  N   (Shift + n)
    HID_KEY_O,          // 79  O   (Shift + o)
    HID_KEY_P,          // 80  P   (Shift + p)
    HID_KEY_Q,          // 81  Q   (Shift + q)
    HID_KEY_R,          // 82  R   (Shift + r)
    HID_KEY_S,          // 83  S   (Shift + s)
    HID_KEY_T,          // 84  T   (Shift + t)
    HID_KEY_U,          // 85  U   (Shift + u)
    HID_KEY_V,          // 86  V   (Shift + v)
    HID_KEY_W,          // 87  W   (Shift + w)
    HID_KEY_X,          // 88  X   (Shift + x)
    HID_KEY_Y,          // 89  Y   (Shift + y)
    HID_KEY_Z,          // 90  Z   (Shift + z)
    HID_KEY_LBRACKET,   // 91  [
    HID_KEY_BACKSLASH,  // 92  \
    HID_KEY_RBRACKET,   // 93  ]
    HID_KEY_6,          // 94  ^   (Shift + 6)
    HID_KEY_MINUS,      // 95  _   (Shift + -)
    HID_KEY_GRAVE,      // 96  `
    HID_KEY_A,          // 97  a
    HID_KEY_B,          // 98  b
    HID_KEY_C,          // 99  c
    HID_KEY_D,          // 100 d
    HID_KEY_E,          // 101 e
    HID_KEY_F,          // 102 f
    HID_KEY_G,          // 103 g
    HID_KEY_H,          // 104 h
    HID_KEY_I,          // 105 i
    HID_KEY_J,          // 106 j
    HID_KEY_K,          // 107 k
    HID_KEY_L,          // 108 l
    HID_KEY_M,          // 109 m
    HID_KEY_N,          // 110 n
    HID_KEY_O,          // 111 o
    HID_KEY_P,          // 112 p
    HID_KEY_Q,          // 113 q
    HID_KEY_R,          // 114 r
    HID_KEY_S,          // 115 s
    HID_KEY_T,          // 116 t
    HID_KEY_U,          // 117 u
    HID_KEY_V,          // 118 v
    HID_KEY_W,          // 119 w
    HID_KEY_X,          // 120 x
    HID_KEY_Y,          // 121 y
    HID_KEY_Z,          // 122 z
    HID_KEY_LBRACKET,   // 123 {   (Shift + [)
    HID_KEY_BACKSLASH,  // 124 |   (Shift + \)
    HID_KEY_RBRACKET,   // 125 }   (Shift + ])
    HID_KEY_GRAVE,      // 126 ~   (Shift + `)
    HID_KEY_DELETE,     // 127 DEL

    // ---- Extended CP437 (128-255) ----
    // Mapped to closest ASCII/Latin-1 equivalents where possible
    HID_KEY_C,          // 128 Ç → C
    HID_KEY_U,          // 129 ü → u
    HID_KEY_E,          // 130 é → e
    HID_KEY_A,          // 131 â → a
    HID_KEY_A,          // 132 ä → a
    HID_KEY_A,          // 133 à → a
    HID_KEY_A,          // 134 å → a
    HID_KEY_C,          // 135 ç → c
    HID_KEY_E,          // 136 ê → e
    HID_KEY_E,          // 137 ë → e
    HID_KEY_E,          // 138 è → e
    HID_KEY_I,          // 139 ï → i
    HID_KEY_I,          // 140 î → i
    HID_KEY_I,          // 141 ì → i
    HID_KEY_A,          // 142 Ä → A
    HID_KEY_A,          // 143 Å → A
    HID_KEY_E,          // 144 É → E
    HID_KEY_A,          // 145 æ → a (ae)
    HID_KEY_A,          // 146 Æ → A (AE)
    HID_KEY_O,          // 147 ô → o
    HID_KEY_O,          // 148 ö → o
    HID_KEY_O,          // 149 ò → o
    HID_KEY_U,          // 150 û → u
    HID_KEY_U,          // 151 ù → u
    HID_KEY_Y,          // 152 ÿ → y
    HID_KEY_O,          // 153 Ö → O
    HID_KEY_U,          // 154 Ü → U
    HID_KEY_O,          // 155 ø → o
    HID_KEY_L,          // 156 £ → L (pound, no direct mapping)
    HID_KEY_O,          // 157 Ø → O
    HID_KEY_X,          // 158 × → x (multiplication sign)
    HID_KEY_F,          // 159 ƒ → f
    HID_KEY_A,          // 160 á → a
    HID_KEY_I,          // 161 í → i
    HID_KEY_O,          // 162 ó → o
    HID_KEY_U,          // 163 ú → u
    HID_KEY_N,          // 164 ñ → n
    HID_KEY_N,          // 165 Ñ → N
    HID_KEY_A,          // 166 ª → a
    HID_KEY_O,          // 167 º → o
    HID_KEY_SLASH,      // 168 ¿ → ? (inverted question → question)
    HID_KEY_R,          // 169 ® → R
    HID_KEY_NONE,       // 170 ¬ (not sign, no key)
    HID_KEY_2,          // 171 ½ → 1/2
    HID_KEY_4,          // 172 ¼ → 1/4
    HID_KEY_1,          // 173 ¡ → ! (inverted exclamation → !)
    HID_KEY_NONE,       // 174 « → << (guillemet left)
    HID_KEY_NONE,       // 175 » → >> (guillemet right)
    HID_KEY_NONE,       // 176 ░ (box drawing - display only)
    HID_KEY_NONE,       // 177 ▒ (box drawing - display only)
    HID_KEY_NONE,       // 178 ▓ (box drawing - display only)
    HID_KEY_NONE,       // 179 │ (box drawing - display only)
    HID_KEY_NONE,       // 180 ┤ (box drawing - display only)
    HID_KEY_A,          // 181 Á → A
    HID_KEY_A,          // 182 Â → A
    HID_KEY_A,          // 183 À → A
    HID_KEY_C,          // 184 © → (c) → C
    HID_KEY_NONE,       // 185 ╣ (box drawing - display only)
    HID_KEY_NONE,       // 186 ║ (box drawing - display only)
    HID_KEY_NONE,       // 187 ╗ (box drawing - display only)
    HID_KEY_NONE,       // 188 ╝ (box drawing - display only)
    HID_KEY_C,          // 189 ¢ → c (cent)
    HID_KEY_Y,          // 190 ¥ → Y (yen)
    HID_KEY_NONE,       // 191 ┐ (box drawing - display only)
    HID_KEY_NONE,       // 192 └ (box drawing - display only)
    HID_KEY_NONE,       // 193 ┴ (box drawing - display only)
    HID_KEY_NONE,       // 194 ┬ (box drawing - display only)
    HID_KEY_NONE,       // 195 ├ (box drawing - display only)
    HID_KEY_NONE,       // 196 ─ (box drawing - display only)
    HID_KEY_NONE,       // 197 ┼ (box drawing - display only)
    HID_KEY_A,          // 198 ã → a
    HID_KEY_A,          // 199 Ã → A
    HID_KEY_NONE,       // 200 ╚ (box drawing - display only)
    HID_KEY_NONE,       // 201 ╔ (box drawing - display only)
    HID_KEY_NONE,       // 202 ╩ (box drawing - display only)
    HID_KEY_NONE,       // 203 ╦ (box drawing - display only)
    HID_KEY_NONE,       // 204 ╠ (box drawing - display only)
    HID_KEY_NONE,       // 205 ═ (box drawing - display only)
    HID_KEY_NONE,       // 206 ╬ (box drawing - display only)
    HID_KEY_NONE,       // 207 ¤ (currency sign)
    HID_KEY_D,          // 208 ð → d (eth)
    HID_KEY_D,          // 209 Ð → D (Eth)
    HID_KEY_E,          // 210 Ê → E
    HID_KEY_E,          // 211 Ë → E
    HID_KEY_E,          // 212 È → E
    HID_KEY_I,          // 213 ı → i (dotless i)
    HID_KEY_I,          // 214 Í → I
    HID_KEY_I,          // 215 Î → I
    HID_KEY_I,          // 216 Ï → I
    HID_KEY_NONE,       // 217 ┘ (box drawing - display only)
    HID_KEY_NONE,       // 218 ┌ (box drawing - display only)
    HID_KEY_NONE,       // 219 █ (full block - display only)
    HID_KEY_NONE,       // 220 ▄ (bottom half block - display only)
    HID_KEY_BACKSLASH,  // 221 ¦ → | (broken bar → pipe)
    HID_KEY_I,          // 222 Ì → I
    HID_KEY_NONE,       // 223 ▀ (top half block - display only)
    HID_KEY_O,          // 224 Ó → O
    HID_KEY_S,          // 225 ß → ss → s
    HID_KEY_O,          // 226 Ô → O
    HID_KEY_O,          // 227 Ò → O
    HID_KEY_O,          // 228 õ → o
    HID_KEY_O,          // 229 Õ → O
    HID_KEY_U,          // 230 µ → u (micro → mu)
    HID_KEY_T,          // 231 þ → th → t (thorn)
    HID_KEY_T,          // 232 Þ → TH → T (Thorn)
    HID_KEY_U,          // 233 Ú → U
    HID_KEY_U,          // 234 Û → U
    HID_KEY_U,          // 235 Ù → U
    HID_KEY_Y,          // 236 ý → y
    HID_KEY_Y,          // 237 Ý → Y
    HID_KEY_MINUS,      // 238 ¯ → - (macron)
    HID_KEY_APOSTROPHE, // 239 ´ → ' (acute accent)
    HID_KEY_EQUAL,      // 240 ≡ → = (identical to)
    HID_KEY_EQUAL,      // 241 ± → +/- → + (plus-minus)
    HID_KEY_NONE,       // 242 ‗ (double underline - display only)
    HID_KEY_3,          // 243 ¾ → 3/4
    HID_KEY_P,          // 244 ¶ → P (pilcrow)
    HID_KEY_S,          // 245 § → S (section)
    HID_KEY_SLASH,      // 246 ÷ → / (division)
    HID_KEY_COMMA,      // 247 ¸ → , (cedilla)
    HID_KEY_NONE,       // 248 ° (degree - no key)
    HID_KEY_APOSTROPHE, // 249 ¨ → " (diaeresis)
    HID_KEY_PERIOD,     // 250 · → . (middle dot)
    HID_KEY_1,          // 251 ¹ → 1 (superscript 1)
    HID_KEY_3,          // 252 ³ → 3 (superscript 3)
    HID_KEY_2,          // 253 ² → 2 (superscript 2)
    HID_KEY_NONE,       // 254 ■ (black square - display only)
    HID_KEY_SPACE,      // 255 NBSP → space
};

// ============================================================================
// CP437 → Shift Modifier Flag Table (PROGMEM)
// ============================================================================
// true = Shift key must be pressed for this character

const uint8_t cp437_hid_shift[] PROGMEM = {
    // Control (0-31): all no shift
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,

    // Printable ASCII (32-126)
    0,  // 32  ' '    no shift
    1,  // 33  !       shift
    1,  // 34  "       shift
    1,  // 35  #       shift
    1,  // 36  $       shift
    1,  // 37  %       shift
    1,  // 38  &       shift
    0,  // 39  '       no shift
    1,  // 40  (       shift
    1,  // 41  )       shift
    1,  // 42  *       shift
    1,  // 43  +       shift
    0,  // 44  ,       no shift
    0,  // 45  -       no shift
    0,  // 46  .       no shift
    0,  // 47  /       no shift
    0,  // 48  0       no shift
    0,  // 49  1       no shift
    0,  // 50  2       no shift
    0,  // 51  3       no shift
    0,  // 52  4       no shift
    0,  // 53  5       no shift
    0,  // 54  6       no shift
    0,  // 55  7       no shift
    0,  // 56  8       no shift
    0,  // 57  9       no shift
    1,  // 58  :       shift
    0,  // 59  ;       no shift
    1,  // 60  <       shift
    0,  // 61  =       no shift
    1,  // 62  >       shift
    1,  // 63  ?       shift
    1,  // 64  @       shift
    1,  // 65  A       shift (uppercase)
    1,  // 66  B       shift
    1,  // 67  C       shift
    1,  // 68  D       shift
    1,  // 69  E       shift
    1,  // 70  F       shift
    1,  // 71  G       shift
    1,  // 72  H       shift
    1,  // 73  I       shift
    1,  // 74  J       shift
    1,  // 75  K       shift
    1,  // 76  L       shift
    1,  // 77  M       shift
    1,  // 78  N       shift
    1,  // 79  O       shift
    1,  // 80  P       shift
    1,  // 81  Q       shift
    1,  // 82  R       shift
    1,  // 83  S       shift
    1,  // 84  T       shift
    1,  // 85  U       shift
    1,  // 86  V       shift
    1,  // 87  W       shift
    1,  // 88  X       shift
    1,  // 89  Y       shift
    1,  // 90  Z       shift
    0,  // 91  [       no shift
    0,  // 92  \       no shift
    0,  // 93  ]       no shift
    1,  // 94  ^       shift
    1,  // 95  _       shift
    0,  // 96  `       no shift
    0,  // 97  a       lowercase
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // 98-113 b-z lowercase
    0,0,0,0,0,0,0,0,0,                  // 114-122
    1,  // 123 {       shift
    1,  // 124 |       shift
    1,  // 125 }       shift
    1,  // 126 ~       shift
    0,  // 127 DEL     no shift (not printable)

    // Extended CP437 (128-255)
    1,  // 128 Ç → shift+C
    0,  // 129 ü → u
    0,  // 130 é → e
    0,  // 131 â → a
    0,  // 132 ä → a
    0,  // 133 à → a
    0,  // 134 å → a
    0,  // 135 ç → c
    0,  // 136 ê → e
    0,  // 137 ë → e
    0,  // 138 è → e
    0,  // 139 ï → i
    0,  // 140 î → i
    0,  // 141 ì → i
    1,  // 142 Ä → shift+A
    1,  // 143 Å → shift+A
    1,  // 144 É → shift+E
    0,  // 145 æ → a
    1,  // 146 Æ → shift+A
    0,  // 147 ô → o
    0,  // 148 ö → o
    0,  // 149 ò → o
    0,  // 150 û → u
    0,  // 151 ù → u
    0,  // 152 ÿ → y
    1,  // 153 Ö → shift+O
    1,  // 154 Ü → shift+U
    0,  // 155 ø → o
    1,  // 156 £ → shift+L
    1,  // 157 Ø → shift+O
    0,  // 158 × → x
    0,  // 159 ƒ → f
    0,  // 160 á → a
    0,  // 161 í → i
    0,  // 162 ó → o
    0,  // 163 ú → u
    0,  // 164 ñ → n
    1,  // 165 Ñ → shift+N
    0,  // 166 ª → a
    0,  // 167 º → o
    1,  // 168 ¿ → shift+/ (?)
    1,  // 169 ® → shift+R
    0,  // 170 ¬ (no key)
    0,  // 171 ½ → 2
    0,  // 172 ¼ → 4
    1,  // 173 ¡ → shift+1 (!)
    0,  // 174 « (no key)
    0,  // 175 » (no key)
    0,  // 176 ░ (display)
    0,  // 177 ▒ (display)
    0,  // 178 ▓ (display)
    0,  // 179 │ (display)
    0,  // 180 ┤ (display)
    1,  // 181 Á → shift+A
    1,  // 182 Â → shift+A
    1,  // 183 À → shift+A
    1,  // 184 © → shift+C (not exact but close)
    0,  // 185 ╣ (display)
    0,  // 186 ║ (display)
    0,  // 187 ╗ (display)
    0,  // 188 ╝ (display)
    0,  // 189 ¢ → c
    1,  // 190 ¥ → shift+Y
    0,  // 191 ┐ (display)
    0,  // 192 └ (display)
    0,  // 193 ┴ (display)
    0,  // 194 ┬ (display)
    0,  // 195 ├ (display)
    0,  // 196 ─ (display)
    0,  // 197 ┼ (display)
    0,  // 198 ã → a
    1,  // 199 Ã → shift+A
    0,  // 200 ╚ (display)
    0,  // 201 ╔ (display)
    0,  // 202 ╩ (display)
    0,  // 203 ╦ (display)
    0,  // 204 ╠ (display)
    0,  // 205 ═ (display)
    0,  // 206 ╬ (display)
    0,  // 207 ¤ (no key)
    0,  // 208 ð → d
    1,  // 209 Ð → shift+D
    1,  // 210 Ê → shift+E
    1,  // 211 Ë → shift+E
    1,  // 212 È → shift+E
    0,  // 213 ı → i
    1,  // 214 Í → shift+I
    1,  // 215 Î → shift+I
    1,  // 216 Ï → shift+I
    0,  // 217 ┘ (display)
    0,  // 218 ┌ (display)
    0,  // 219 █ (display)
    0,  // 220 ▄ (display)
    1,  // 221 ¦ → shift+\ (|)
    1,  // 222 Ì → shift+I
    0,  // 223 ▀ (display)
    1,  // 224 Ó → shift+O
    0,  // 225 ß → s
    1,  // 226 Ô → shift+O
    1,  // 227 Ò → shift+O
    0,  // 228 õ → o
    1,  // 229 Õ → shift+O
    0,  // 230 µ → u
    0,  // 231 þ → t
    1,  // 232 Þ → shift+T
    1,  // 233 Ú → shift+U
    1,  // 234 Û → shift+U
    1,  // 235 Ù → shift+U
    0,  // 236 ý → y
    1,  // 237 Ý → shift+Y
    0,  // 238 ¯ → -
    0,  // 239 ´ → '
    0,  // 240 ≡ → =
    0,  // 241 ± → +
    0,  // 242 ‗ (display)
    0,  // 243 ¾ → 3 (mapped to digit)
    0,  // 244 ¶ → p
    0,  // 245 § → s
    0,  // 246 ÷ → /
    0,  // 247 ¸ → ,
    0,  // 248 ° (no key)
    0,  // 249 ¨ → '
    0,  // 250 · → .
    0,  // 251 ¹ → 1
    0,  // 252 ³ → 3
    0,  // 253 ² → 2
    0,  // 254 ■ (display)
    0,  // 255 NBSP → space
};

// ============================================================================
// AltGr Modifier Flag Table (PROGMEM)
// ============================================================================
// true = AltGr (Right Alt) needed for this character on US-Intl keyboards

const uint8_t cp437_hid_altgr[] PROGMEM = {
    // 0-255: all false for US standard layout
    // Extended characters that need AltGr on some layouts are
    // handled via the simplified ASCII mapping above.
    // This table exists for future international keyboard support.
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

// ============================================================================
// CP437 Glyph Names (for display on screen)
// ============================================================================
// Human-readable character or description for display rendering
// Stored in flash for showing on the ST7789 HID display

const char cp437_names[] PROGMEM =
    "NUL\0SOH\0STX\0ETX\0EOT\0ENQ\0ACK\0BEL\0"
    "BS\0 HT\0 LF\0 VT\0 FF\0 CR\0 SO\0 SI\0"
    "DLE\0DC1\0DC2\0DC3\0DC4\0NAK\0SYN\0ETB\0"
    "CAN\0EM\0 SUB\0ESC\0FS\0 GS\0 RS\0 US\0"
    " \0!\0\"\0#\0$\0%\0&\0'\0(\0)\0*\0+\0,\0-\0.\0/\0"
    "0\001\0""2\0""3\0""4\0""5\0""6\0""7\0""8\0""9\0:\0;\0<\0=\0>\0?\0"
    "@\0A\0B\0C\0D\0E\0F\0G\0H\0I\0J\0K\0L\0M\0N\0O\0"
    "P\0Q\0R\0S\0T\0U\0V\0W\0X\0Y\0Z\0[\0\\\0]\0^\0_\0"
    "`\0a\0b\0c\0d\0e\0f\0g\0h\0i\0j\0k\0l\0m\0n\0o\0"
    "p\0q\0r\0s\0t\0u\0v\0w\0x\0y\0z\0{\0|\0}\0~\0DEL\0"
    // Extended characters use a separate lookup
;

// ============================================================================
// Helper: Convert CP437 byte to HID keycode + modifier
// ============================================================================
inline void cp437_to_hid(uint8_t cp, uint8_t *modifier, uint8_t *keycode) {
    if (cp > 255) cp = '?'; // Fallback
    *keycode = pgm_read_byte(&cp437_hid_key[cp]);
    uint8_t shift = pgm_read_byte(&cp437_hid_shift[cp]);
    uint8_t altgr = pgm_read_byte(&cp437_hid_altgr[cp]);
    *modifier = (shift ? HID_MOD_LEFTSHIFT : 0) | (altgr ? HID_MOD_RIGHTALT : 0);
}

// ============================================================================
// Helper: Check if a CP437 character is a display-only glyph (box drawing, blocks)
// ============================================================================
inline bool cp437_is_display_only(uint8_t cp) {
    // Box drawing characters and block elements are display-only
    return (cp >= 0xB0 && cp <= 0xDF) ||  // 176-223: box drawing + blocks
           cp == 0xFE ||                   // 254: black square
           cp == 0xFA || cp == 0xF4;       // 250, 244: middle dot, pilcrow (debatable)
}

// ============================================================================
// Helper: Check if a CP437 character is printable (has a visual representation)
// ============================================================================
inline bool cp437_is_printable(uint8_t cp) {
    return cp >= 0x20 && cp != 0x7F; // Not control and not DEL
}

#endif // CP437_MAP_H