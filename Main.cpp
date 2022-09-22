/*

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

*/

#include <vcl.h>
#pragma hdrstop

#include "Main.h"
SYS sys;
// ---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

//---------------------------------------------------------------------------
void __fastcall SetDirName(LPSTR t, LPCSTR pName)
{
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char name[_MAX_FNAME];
        char ext[_MAX_EXT];
        AnsiString      Dir;

        ::_splitpath( pName, drive, dir, name, ext );
        Dir = drive;
        Dir += dir;
        strncpy(t, Dir.c_str(), MAX_PATH-1);
}
//---------------------------------------------------------------------------
void __fastcall SetEXT(LPSTR t, LPCSTR pExt)
{
        LPSTR p = LASTP(t);
        for( ; p > t; p-- ){
                if( *p == '.' ){
                        p++;
                        strcpy(p, pExt);
                        return;
                }
        }
}
//---------------------------------------------------------------------------
WORD __fastcall htow(LPCSTR p)
{
        int d = 0;
        for( ; *p; p++ ){
                if( (*p != ' ') && (*p != '$') ){
                        d = d << 4;
                        d += *p & 0x0f;
                        if( *p >= 'A' ) d += 9;
                }
        }
        return WORD(d);
}
//---------------------------------------------------------------------------

bool Is64bitOS(){
        char *ptr = getenv("ProgramW6432");
        if( ptr != NULL )
                return true;
        else
                return false;
}

//***************************************************************************
// CDLPort class
__fastcall CDLPort::CDLPort(LPCSTR pPath)
{
        m_hDLib = NULL;
        m_Path = pPath;
        LPSTR p = LASTP(m_Path.c_str());
        if (*p == '\\')
                * p = 0;
}
//---------------------------------------------------------------------------
__fastcall CDLPort::~CDLPort()
{
        /*
          if( m_hLib != NULL ){
          Close();
          ::FreeLibrary(m_hLib);
          m_hLib = NULL;
          }
        */
        if( m_hDLib != NULL ){
                ::FreeLibrary(m_hDLib);
                m_hDLib = NULL;
        }
}
//---------------------------------------------------------------------
BOOL __fastcall CDLPort::IsFile(LPCSTR pName)
{
        HANDLE hFile = ::CreateFile(pName,
                                    0,
                                    FILE_SHARE_READ, NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL
                                    );
        if( hFile != INVALID_HANDLE_VALUE ){
                ::CloseHandle(hFile);
        }
        return hFile != INVALID_HANDLE_VALUE;
}
//---------------------------------------------------------------------------
//***************************************************************************
// CFSK class
__fastcall CFSK::CFSK(void)
{
        Init();
#if MeasureAccuracy
        QueryPerformanceFrequency(&m_liFreq);
        m_liPOld.u.HighPart = -1;
#endif
}
//---------------------------------------------------------------------------
void __fastcall CFSK::Init(void)
{
        m_hPort = INVALID_HANDLE_VALUE;
        m_StgD = -1;
        m_nFSK = 0;
        m_nPTT = 2;
        m_Sequence = 0;
        m_Count = 0;
        m_oFSK = 1;
        m_oPTT = 0;
        m_shift_state = 0;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall CFSK::SetPTT(int sw, TMemo* Memo)
{
        if( sw )
                tinyIt( '[', Memo );
        else
                tinyIt( ']', Memo );
        if( m_oPTT != sw ){
                m_oPTT = sw;
                if( sw ){
                        SetDelay(8);
                }
        }
}
//---------------------------------------------------------------------------
const int ttable0[32] = {
        0x00,   //0 null is null
        'E',    //1
        0x0A,   //2
        'A',    //3
        ' ',    //4
        'S',    //5
        'I',    //6
        'U',    //7
        '}',    //8
        'D',    //9
        'R',    //10
        'J',    //11
        'N',    //12
        'F',    //13
        'C',    //14
        'K',    //15
        'T',    //16
        'Z',    //17
        'L',    //18
        'W',    //19
        'H',    //20
        'Y',    //21
        'P',    //22
        'Q',    //23
        'O',    //24
        'B',    //25
        'G',    //26
        0x00,   //27 figure
        'M',    //28
        'X',    //29
        'V',    //30
        0x00    //31 letter
};
const int ttable1[32] = {
        0x00,   //0 null is null
        '3',    //1
        0x0A,   //2
        '-',    //3
        ' ',    //4
        '\'',   //5
        '8',    //6
        '7',    //7
        '}',    //8
        '$',    //9
        '4',    //10
        '\'',   //11
        ',',    //12
        '!',    //13
        ':',    //14
        '(',    //15
        '5',    //16
        '+',    //17
        ')',    //18
        '2',    //19
        0x00,   //20
        '6',    //21
        '0',    //22
        '1',    //23
        '9',    //24
        '?',    //25
        '&',    //26
        0x00,   //27 figure
        '.',    //28
        '/',    //29
        ';',    //30
        0x00    //31 letter
};

BYTE __fastcall CFSK::baudot2ascii( BYTE c )
{
        if( m_shift_state == 0 ){
                return( ttable0[c] );
        }
        else{
                return( ttable1[c] );
        }
}

BOOL __fastcall CFSK::tinyIt( BYTE c, TMemo * Memo )
{
        BYTE d;
        BOOL ret;
        switch( c ){
                case 0x1f: m_shift_state = 0; return TRUE;
                case 0x1b: m_shift_state = 1; return TRUE;
                case 0x00: return TRUE;  // Ignore BLANK
                case 0x02: return TRUE;  // Ignore LF. Use WK } for CR
                case '[': d = c; break;
                case ']': d = c; break;
                default: d = baudot2ascii(c & 0x1F); break;
        }

        if ( !m_oPTT && d == ']' )
                return TRUE;

        DWORD sent;
#ifndef NDEBUG
        char bf[128];
        if ( Memo ) {
                wsprintf(bf, "tinyIt: %02X", d);
                Memo->Lines->Add(bf);
        }
#endif
        ret = WriteFile( m_hPort, &d, 1, &sent, NULL );

#ifndef NDEBUG
        if ( Memo ) {
                Memo->Lines->Add("RTTY Cmd write status " + AnsiString(ret) +
                " sent count " + AnsiString(sent));
                printWKstatus(Memo);
                if (!ret)
                        ErrorExit(TEXT("tinyIt"));
        }
#endif

        return ret;
}

void __fastcall CFSK::printWKstatus( TMemo * Memo )
{
        BYTE hostcmd[2];
        DWORD sentlength;
        DWORD readlength;
        hostcmd[0] = 0x15;
        BYTE response[512];
        char bf[128];

#ifndef NDEBUG
        WriteFile( m_hPort, hostcmd, 1, &sentlength, NULL );
        Sleep(50);

        ReadFile( m_hPort, &response, 511, &readlength, NULL );

        if (readlength == 1) {
                wsprintf(bf, "WK status: %02X", response[0]);
                Memo->Lines->Add(bf);
        }
        else {
                response[readlength] = 0x00;
                wsprintf(bf, "Response length %d: %s", readlength, response);
                Memo->Lines->Add(bf);
        }
#endif
}
//---------------------------------------------------------------------------
void CFSK::ErrorExit(LPTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    snprintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}
//***************************************************************************
// TExtFSK (MainWindow) class
//---------------------------------------------------------------------------
__fastcall TExtFSK::TExtFSK(TComponent* Owner)
        : TForm(Owner)
{
        m_DisEvent = 1;
        Top = 0;
        Left = 0;
        m_hPort = INVALID_HANDLE_VALUE;
        m_X = 0;

        m_IniName = sys.m_ModuleName;
        SetEXT(m_IniName.c_str(), "ini");
        PortName->ItemIndex = 0;
        ReadIniFile();
        m_WindowState = WindowState;

        OpenPort();
        UpdatePort();
        m_DisEvent = 0;

        AnsiString cap = "WinKeyer FSK ";
        cap += VERSION;
        Caption = cap;
        SetPTT(0, FALSE);
}
//---------------------------------------------------------------------------

void __fastcall TExtFSK::FormClose(TObject *Sender, TCloseAction &Action)
{
        ClosePort();
        WriteIniFile();
}
//---------------------------------------------------------------------------
void __fastcall TExtFSK::ReadIniFile(void)
{
        TMemIniFile *pIniFile = new TMemIniFile(m_IniName);

        Top = pIniFile->ReadInteger("Window", "Top", Top);
        Left = pIniFile->ReadInteger("Window", "Left", Left);
        WindowState = (TWindowState)pIniFile->ReadInteger("Window", "State", WindowState);
        AnsiString as = pIniFile->ReadString("Settings", "Port", "COM1");
        int n = PortName->Items->IndexOf(as);
        if( n < 0 ){
                n = atoi(as.c_str());
                if( n < 0 ) n = 0;
        }
        PortName->ItemIndex = n;
        RGDiddle->ItemIndex = pIniFile->ReadInteger("Settings", "Diddle", RGDiddle->ItemIndex);
        RGStopBits->ItemIndex = pIniFile->ReadInteger("Settings", "StopBits", RGStopBits->ItemIndex);
        CBReverse->Checked = pIniFile->ReadInteger("Settings", "Reverse", CBReverse->Checked);
        CBFSKMap->Checked = pIniFile->ReadInteger("Settings", "FSKMAP", CBFSKMap->Checked);
        CBUSOS->Checked = pIniFile->ReadInteger("Settings", "USOS", CBUSOS->Checked);
        CBAutoCRLF->Checked = pIniFile->ReadInteger("Settings", "AutoCRLF", CBAutoCRLF->Checked);
        delete pIniFile;
}
//---------------------------------------------------------------------------
void __fastcall TExtFSK::WriteIniFile(void)
{
        TMemIniFile *pIniFile = new TMemIniFile(m_IniName);
        pIniFile->WriteInteger("Window", "Top", Top);
        pIniFile->WriteInteger("Window", "Left", Left);
        pIniFile->WriteInteger("Window", "State", WindowState);
        pIniFile->WriteString("Settings", "Port", PortName->Items->Strings[PortName->ItemIndex]);
        pIniFile->WriteInteger("Settings", "Diddle", RGDiddle->ItemIndex);
        pIniFile->WriteInteger("Settings", "StopBits", RGStopBits->ItemIndex);
        pIniFile->WriteInteger("Settings", "Reverse", CBReverse->Checked);
        pIniFile->WriteInteger("Settings", "FSKMAP", CBFSKMap->Checked);
        pIniFile->WriteInteger("Settings", "USOS", CBUSOS->Checked);
        pIniFile->WriteInteger("Settings", "AutoCRLF", CBAutoCRLF->Checked);
        pIniFile->UpdateFile();
        delete pIniFile;
}
//---------------------------------------------------------------------------
void __fastcall TExtFSK::UpdatePort()
{
        if( sys.m_WinNT && !sys.m_pDLPort ) return;

        m_DisEvent++;
        m_DisEvent--;
}
//---------------------------------------------------------------------------
void __fastcall TExtFSK::UpdateComStat(void)
{
        char bf[128];

        wsprintf(bf, "Status:%s", m_hPort != INVALID_HANDLE_VALUE ? "OK" : "NG");
        LComStat->Color = m_hPort != INVALID_HANDLE_VALUE ? clBtnFace : clRed;
        LComStat->Caption = bf;
}
//---------------------------------------------------------------------------
void __fastcall TExtFSK::OpenPort(void)
{
        ClosePort();
        OpenPort_();
        UpdateComStat();
        m_fsk.m_hPort = m_hPort;
}
//---------------------------------------------------------------------------
BOOL __fastcall TExtFSK::OpenPort_(void)
{
        AnsiString pname = PortName->Items->Strings[PortName->ItemIndex];

        if( pname.SubString(1,3) == "COM" ){
                pname = "\\\\.\\" + pname;
        }
        else {
                m_hPort = INVALID_HANDLE_VALUE;
                return FALSE;
        }

        m_hPort = ::CreateFile( pname.c_str(),
                                GENERIC_READ | GENERIC_WRITE,
                                0, NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );
        if( m_hPort == INVALID_HANDLE_VALUE )
                return FALSE;
        if( ::SetupComm( m_hPort, DWORD(256), DWORD(2) ) == FALSE ){
                ::CloseHandle(m_hPort);
                m_hPort = INVALID_HANDLE_VALUE;
                return FALSE;
        }
        ::PurgeComm( m_hPort,
                PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR );

        COMMTIMEOUTS TimeOut;

        TimeOut.ReadIntervalTimeout = 1000;
        TimeOut.ReadTotalTimeoutMultiplier = 20;
        TimeOut.ReadTotalTimeoutConstant = 100;
        TimeOut.WriteTotalTimeoutMultiplier = 0;
        TimeOut.WriteTotalTimeoutConstant = 20000;
        if( !::SetCommTimeouts( m_hPort, &TimeOut ) ){
                ::CloseHandle( m_hPort );
                m_hPort = INVALID_HANDLE_VALUE;
                return FALSE;
        }
        ::GetCommState( m_hPort, &m_dcb );
        m_dcb.BaudRate = 1200;
        m_dcb.fBinary = TRUE;
        m_dcb.ByteSize = 8;
        m_dcb.Parity = NOPARITY;
        m_dcb.StopBits = TWOSTOPBITS;
        m_dcb.XonChar = 0x11;   // XON
        m_dcb.XoffChar = 0x13;  // XOFF
        m_dcb.fParity = 0;
        m_dcb.fOutxCtsFlow = FALSE;
        m_dcb.fInX = m_dcb.fOutX = FALSE;
        m_dcb.fOutxDsrFlow = FALSE;
        m_dcb.EvtChar = 0x0d;
        m_dcb.fRtsControl = RTS_CONTROL_DISABLE;
        m_dcb.fDtrControl = DTR_CONTROL_DISABLE;
        m_dcb.fTXContinueOnXoff = FALSE;
        m_dcb.XonLim = USHORT(256/4);
        m_dcb.XoffLim = USHORT(256*3/4);
        m_dcb.DCBlength = sizeof( DCB );

        if( !::SetCommState( m_hPort, &m_dcb ) ){
                ::CloseHandle( m_hPort );
                m_hPort = INVALID_HANDLE_VALUE;
                return FALSE;
        }

        if( !::SetCommMask( m_hPort, EV_RXFLAG ) ){
                ::CloseHandle(m_hPort);
                m_hPort = INVALID_HANDLE_VALUE;
                return FALSE;
        }

        // Talk to WinKeyer and open it for communications
        Show();
        Memo->Lines->Add("Connecting to WinKeyer...");

        BYTE hostcmd[2];
        char response[512];
        DWORD sentlength;
        DWORD readlength;
        char statusMessage[512];

        hostcmd[0] = 0x00;
        hostcmd[1] = 0x02;  // WK Host Open

        WriteFile( m_hPort, hostcmd, 2, &sentlength, NULL );

        ReadFile( m_hPort, &response, 512, &readlength, NULL );
        response[readlength] = 0x00;

        if (readlength == 1) {
                int ver_response;
                int ver_major;
                int ver_minor;
                unsigned ver_release = 0;

                ver_response = (int) response[0];
                ver_major = ver_response / 10;
                ver_minor = ver_response % 10;

                hostcmd[0] = 0x00;
                hostcmd[1] = 0x17;  // WK Get Firmware Minor Ver

                WriteFile( m_hPort, hostcmd, 2, &sentlength, NULL );

                ReadFile( m_hPort, &response, 512, &readlength, NULL );
                response[readlength] = 0x00;

                if (readlength == 1)
                        ver_release = (unsigned) response[0];
                if (ver_response >= 31)
                        snprintf(statusMessage, 511,
                                "WinKeyer OK. Version %d.%d Rev %d\n",
                                ver_major, ver_minor, ver_release);
                else
                        snprintf(statusMessage, 511,
                                "WRONG WinKeyer firmware version! RTTY only works with v3.1 or newer. Connected version %d.%d Rev %d\n",
                                ver_major, ver_minor, ver_release);

                statusMessage[511] = 0x00;
                Memo->Lines->Add(statusMessage);
        }
        else {
                if (readlength == 0)
                        Memo->Lines->Add("WinKeyer NOT responding.");
                else {
                        Memo->Lines->Add("WinKeyer NOT responding as expected.");
                        Memo->Lines->Add("Message received:");
                        Memo->Lines->Add(response);
                }
        }

        return TRUE;
}
//---------------------------------------------------------------------------
void __fastcall TExtFSK::ClosePort(void)
{
        BYTE hostcmd[2];
        DWORD sentlength;

        hostcmd[0] = 0x00;
        hostcmd[1] = 0x03;  // WK Host Close

        WriteFile( m_hPort, hostcmd, 2, &sentlength, NULL );

        if( IsOpen() ){
                m_fsk.Disable();
                ::CloseHandle(m_hPort);
                m_hPort = INVALID_HANDLE_VALUE;
        }
        UpdateComStat();
        m_fsk.m_hPort = m_hPort;
}
//---------------------------------------------------------------------------
//      para:   Upper16bits     Speed(eg. 45)
//                      Lower16bits     b1-b0   Stop (0-1, 1-1.5, 2-2)
//                                              b5-b2   Length
void __fastcall TExtFSK::SetPara()
{
        int priorPTTstate;
        char ss[32];
        BYTE data[4];
        DWORD sent;
        int s_baud;

        // Close PTT and abort any comms in progress before setting
        // or changing WK params, then reopen
        priorPTTstate = m_ptt;
        if ( m_ptt ) {
                //data[0] = 0x5c; // \ command (abort comms)
                //WriteFile( m_hPort, data, 1, &sent, NULL );
                SetPTT(0, TRUE);
        }

        m_baud = m_para >> 16;

        data[0] = 0x00;  // WK Admin
        data[1] = 0x13;  // WK RTTY Mode Enable
        data[2] = data[3] = 0x00;

        // WK RTTY Params
        // RTTY Enable and Baud rate
        switch( m_baud ){
        case 45:  data[2] = 0x80;  s_baud = 45;  strcpy( ss, "45.45" ); break;
        case 50:  data[2] = 0x81;  s_baud = 50;  strcpy( ss, "50" ); break;
        case 75:  data[2] = 0x82;  s_baud = 75;  strcpy( ss, "75" ); break;
        case 100: data[2] = 0x83;  s_baud = 100;  strcpy( ss, "100" ); break;
        default:  data[2] = 0x80;  s_baud = 45;  strcpy( ss, "45.45" ); break;
        }

        // Diddle or not
        switch( RGDiddle->ItemIndex ) {
                case diddleNONE:
                        data[3] = 0x00;
                        break;
                case diddleBLANKS:
                        data[2] |= 0x40;
                        data[3] = 0x00;
                        break;
                case diddleLTTRS:
                        data[2] |= 0x40;
                        data[3] = 0x04;
                        break;
        }

        // Stop bits, only choices supported by WK are 2 or 1.5
        if ( RGStopBits->ItemIndex == stopbits15 )
                data[3] |= 0x08;

        // Reverse Mark/Space
        if ( CBReverse->Checked )
                data[2] |= 0x04;

        // FSKMAP (Swap FSK and PTT pins)
        if ( CBFSKMap->Checked )
                data[2] |= 0x20;

        // USOS
        if ( CBUSOS->Checked )
                data[3] |= 0x01;

        // AUTOCRLF
        if ( CBAutoCRLF->Checked )
                data[2] |= 0x10;

        int ret;
        ret = WriteFile( m_hPort, data, 4, &sent, NULL );

#ifndef NDEBUG
        Memo->Lines->Add("RTTY Cmd write status " + AnsiString(ret) +
                " sent count " + AnsiString(sent));
        char bf[128];
        wsprintf(bf, "%02X", data[0]);
        Memo->Lines->Add(bf);
        wsprintf(bf, "%02X", data[1]);
        Memo->Lines->Add(bf);
        wsprintf(bf, "%02X", data[2]);
        Memo->Lines->Add(bf);
        wsprintf(bf, "%02X", data[3]);
        Memo->Lines->Add(bf);

        m_fsk.printWKstatus(Memo);


        
        BYTE teststr[64] = "[ALABAMA01234567890123456789012345678901234567]]]";

        ret = WriteFile( m_hPort, teststr, 47, &sent, NULL );
        Memo->Lines->Add("Test write status " + AnsiString(ret) +
                " sent count " + AnsiString(sent));
#endif

        strcat( ss, " baud" );
        LabelBaud->Caption = ss;
        if( m_baud != s_baud )
                LabelBaud->Font->Color = clRed;
        else
                LabelBaud->Font->Color = clBlack;

        if ( priorPTTstate ) {
                SetPTT(1, TRUE);
        }
        m_fsk.printWKstatus(Memo);
}
//---------------------------------------------------------------------------
//PTT off is called when a parameter is changed in MMTTY
void __fastcall TExtFSK::SetPTT(int sw, int msg)
{
        m_ptt = sw;
        m_fsk.SetPTT(sw, Memo);
        m_X = 0;
        if( msg && CBDebugOutput->Checked ){
                if( m_WindowState == wsMinimized) return;
                Memo->Lines->Add(sw ? "PTT ON" : "PTT OFF");
        }
}
//---------------------------------------------------------------------------

void __fastcall TExtFSK::PutChar(BYTE c)
{

        if( !m_ptt ) return;
        double sleep_time;
        if( c == 0x1f || c == 0x1b )
                sleep_time = 0.0;
        else
                sleep_time = ( 1.0/(float)m_baud ) * 1000.0 * 7.45;// * 0.80;
        m_fsk.m_StgD = c;
        m_fsk.tinyIt(c, Memo);
        Sleep( (int)sleep_time );
        m_fsk.m_StgD = -1;
        if( m_WindowState == wsMinimized || ! CBDebugOutput->Checked ) return;

#ifndef PRINT_ORIGINAL_CHAR
        if( c == 0x1f ){
                m_shift_state = 0;
                return;
        }
        else if( c == 0x1b ){
                m_shift_state = 1;
                return;
        }
        else if( c == 0x02 ){ // Ignore LF, only process CR using WK '}'
                return;
        }
        else if( c == 0x00 ){ // Ignore BLANK
                return;
        }

        c = m_fsk.baudot2ascii(c);
#endif
        char bf[128];
        if( m_X ){
                int n = Memo->Lines->Count;
                if( n ) n--;
                strcpy(bf, Memo->Lines->Strings[n].c_str());
                wsprintf(&bf[strlen(bf)], " %02X", c);
                if( !m_ptt ) return;
                Memo->Lines->Strings[n] = bf;
        }
        else {
                wsprintf(bf, "%02X", c);
                Memo->Lines->Add(bf);
        }
        m_X++;
        if( m_X >= 8 ) m_X = 0;
#if MeasureAccuracy
        int d = int(m_fsk.GetPDiff() * 100000 / m_fsk.GetPFreq());
        if( d ){
                wsprintf(bf, "%u.%02ums", d / 100, d % 100);
                Caption = bf;
        }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TExtFSK::PortNameChange(TObject *Sender)
{
        if( m_DisEvent ) return;

        OpenPort();
        UpdatePort();
        SetPara();
}
//---------------------------------------------------------------------------
void __fastcall TExtFSK::SBMinClick(TObject *Sender)
{
        if( m_DisEvent ) return;

        WindowState = wsMinimized;
        m_WindowState = wsMinimized;

        Memo->Lines->Clear();
        m_X = 0;
}

//---------------------------------------------------------------------------
void __fastcall TExtFSK::FormPaint(TObject *Sender)
{
        m_WindowState = WindowState;
}
//---------------------------------------------------------------------------


void __fastcall TExtFSK::CBClick(TObject *Sender)
{
        if( m_DisEvent ) return;

        m_DisEvent++;
        SetPara();
        m_DisEvent--;        
}
//---------------------------------------------------------------------------

