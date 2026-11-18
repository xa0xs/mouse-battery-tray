# Protocols

How each family is read. Byte offsets count the **report ID as byte 0**, the
way Windows hands reports to applications (WebHID leaves it out, so offsets
in a web driver's JavaScript are one lower).

Every exchange here is a read-only query that the vendor's own software sends
all the time. Nothing writes settings to the mouse.

## 0x83 feature exchange: Attack Shark Ultra series, WLmouse, Lamzu, G-Wolves, Glorious, MCHOSE A5

Taken from the Attack Shark R5 Ultra web driver itself
(`www.xvalleyinno.top/AttackShark`, "DriverCore", function `getBatPer`):

```js
r[2]=2, r[3]=2, r[5]=131;          // 64-byte data, sendFeatureReport(0, r)
yield t.setReport(r), yield t.mySleep(100), o = yield t.getReport(n);
o[1]==161 && o[4]==2 && o[6]==131 ? [o[7], o[8]]   // report ID kept
: o[0]==161 && o[3]==2 && o[5]==131 ? [o[6], o[7]] // report ID stripped
: [0, 0]
```

| | bytes |
|---|---|
| request (feature, 65 bytes) | `00 00 00 02 02 00 83 00 ...` |
| reply (feature, report 0) | `00 A1 00 02 02 00 83 <charging> <level>` |

`A1` = awake, `A2` = waking, `A4` = asleep. The driver picks the collection
whose feature report is 64 bytes (65 with the ID), which is how this app picks
it too. The same exchange is confirmed on hardware for the WLmouse Beast X Max,
Lamzu Maya X, Glorious Model O and MCHOSE A5 Pro Max (see credits). The
charging/level order is the same in all of them.

## Attack Shark Beken dongle (VID 1D57: X3, X3 Max, X6, X11, R1)

Nothing is sent. The dongle pushes `03 <model> 40 <sub> <level>` about every
2 seconds on collection `000A:0000`.

- model `0x10` (X6/R1 generation): level in steps of ten, no charging state.
- `sub = 03`: charging (no level). `sub = 02`: full, except the X3 Max
  (model `0x4E`), which sends it while charging.
- otherwise `level` is a percentage.

## MCHOSE

| Chip | Exchange |
|---|---|
| RealTek (VID 5253 / 3837: M7 Ultra, A7 V2 Ultra) | feature report `0x11` (or `0x12`), payload `06` with every byte inverted (`11 F9 FF FF ...`). Inverted reply: `<vid LE> <model LE> ... <flags> <level> <charging>` |
| RealTek (L7 Pro) | pushed report `13 1D ...`, XOR `FF`: level at byte 5, wired flag at byte 4 |
| YJX (A8A5:2255, G7) | output report `00 55 30 A5 0B 2E 01 01 01`, reply `AA 30 ...` level at +8, charging at +9 |
| A5 Pro Max (VID 2023) | the 0x83 exchange above |

## Compx / Nordic 52840 (ATK, VXE, VGN, Pulsar, Scyrox, Dareu, Hitscan, Zaopin)

17-byte output report `08 04 00 .. 00 49` on collection `FF02:0002`. Reply
`08 04 00 00 00 <st> <level> <cable> <mV hi> <mV lo> .. <sum>`. All 17 bytes
sum to `0x55`.

## Compx / Nordic 54L15 (ATK Zero)

64-byte output report `08 7D 72 02 00 <link> 07 01`, where link is `01` for
the dongle and `00` for the cable. Reply `08 72 <status> .. 07 01 <level>`,
with status `00` when the mouse answered.

## Razer

90-byte feature report behind report ID 0: transaction ID (`1F`, `3F`, `FF`
or `9F`, depending on the model), size `02`, class `07`, command `80`
(level 0-255) or `84` (charging), CRC = XOR of bytes 2-87. Status `02` = OK,
`01` = busy, `04` = asleep, `05` = not supported.

## Logitech HID++ 2.0

Long report `11 <device> <feature index> <fn|sw> ...` on `FF00:0002`. The
root feature (index 0) finds the index of `0x1004` (unified battery),
`0x1000` (battery status) or `0x1001` (voltage, converted with Solaar's
discharge curve). `0x0005` gives the name and type. Device indexes 1-6 are
receiver slots, and `FF` is a cabled or Bluetooth device.

## Bluetooth

The battery level Windows itself keeps for Bluetooth devices (device property
`{104EA319-6EE2-4701-BD47-8DDBF425BBE5} 2`, shown in Settings > Bluetooth &
devices). Devices are matched to a HID mouse by container ID.

## Credits

The protocols were worked out by these projects. Thank you!

- [HeyOkay/HaloBattery](https://github.com/HeyOkay/HaloBattery) (MIT): Attack Shark/Inphic, MCHOSE, Razer, Glorious, Lamzu, VAXEE and many more, with hardware confirmations
- [Fan4Metal/mouse_tray](https://github.com/Fan4Metal/mouse_tray) and [Fan4Metal/ATK_tray](https://github.com/Fan4Metal/ATK_tray): Compx Nordic 52840 / 54L15, MCHOSE L7 Pro push
- [incconutwo/mouse-battery-tray](https://github.com/incconutwo/mouse-battery-tray) (MIT): WLmouse
- [HarukaYamamoto0/attack-shark-x11-driver](https://github.com/HarukaYamamoto0/attack-shark-x11-driver): Attack Shark event format
- [OpenRazer](https://github.com/openrazer/openrazer), [Solaar](https://github.com/pwr-Solaar/Solaar): Razer and Logitech
- [andrewrabert/python-pulsar-mouse-tool](https://github.com/andrewrabert/python-pulsar-mouse-tool): Compx framing
