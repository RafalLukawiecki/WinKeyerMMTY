# WinKeyer FSK for MMTTY

This is a source code repository of WinKeyerMMTY FSK extension. It supports
[WinKeyer](https://www.k1elsystems.com) devices containing WinKeyer [firmware
revision 3.1](https://www.k1elsystems.com/WinKeyer_31.html) or later. Those
devices are capable of keying RTTY FSK signal.

This extension also works with [DXLog](http://dxlog.net), a contest logger. You can use different decoders with DXLog: MMTTY, MMVARI, 2Tone, GRITTY. However, with this extension and a WinKeyer, you can dedicate MMTTY to do all the FSK sending.

## Why?

If your transceiver supports FSK you may want to use it instead of generating audio-based AFSK and then modulating it using LSB. Why? Many reasons: to use your transceiver's special FSK filters, to avoid having to configure any Tx audio levels anywhere, not to have to think about frequency offsets (mark is mark), or just to have fun and see how good is your radio's FSK.

Ready? You need a way to key the radio's FSK input, i.e., to convert text into a perfectly-timed, jitter-free sequence of RTTY ITA2/Baudot mark and space symbols. There are a few hardware interfaces around, one of which is WinKeyer 3.1. You still need to interface it with MMTTY. This extension does exactly that. It is **only for Tx,** not for reception. To receive, you still rely on the demodulator in MMTTY or other programs, or in your radio.

## How to Use

1. Copy [WinKeyer.fsk](https://github.com/RafalLukawiecki/WinKeyerMMTY/releases/latest/download/WinKeyer.fsk) file to the directory containing your installation of
MMTTY.
1. Select `WinKeyer` from the `PTT & FSK` section of the MMTTY `Tx` configuration
tab.
1. When the extension starts, it will show a small config and status
window. Select the COM port on which your WinKeyer is running.

Please note, the baud rate is taken from the MMTTY
configuration settings.  Remaining settings need to be configured in the WinKeyerFSK 
window, as they are not read from MMTTY.

<img width="349" alt="WinKeyer FSK MMTTY Config" src="https://user-images.githubusercontent.com/22912705/191830132-8bfae0de-d176-4f9a-94dc-30f4c023332c.png">

## How to Build

Borland C++ Builder version 5.0 is needed to build this project. It is ancient, but it still works well if you can find it.

After a successful build, you will find `WinKeyerMMTY.dll`. Rename it to
`WinKeyer.fsk`.

## History

This project is based on the tinyFSK project by Nobuyuki Oba JA7UDE, who, in turn,
based his work on the EXTFSK64 project.  There is still much unnecessary code, and
the overall code structure is much too complicated.  Apologies from all
contributors for a rather dirty source code.

## Copyright and License

Copyright 2000-2022 Makoto Mori, Nobuyuki Oba, Rafal Lukawiecki

This file is part of WinKeyerMMTY FSK.

WinKeyerMMTTY FSK is free software: you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

WinKeyerMMTTY FSK is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with WinKeyer FSK for MMTTY, see files COPYING and COPYING.LESSER. If not, see
http://www.gnu.org/licenses/.
