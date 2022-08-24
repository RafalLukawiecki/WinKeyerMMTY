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


#ifndef MainH
#define MainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <Inifiles.hpp>
#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>

//---------------------------------------------------------------------------
#define VERSION "0.3"
#define NDEBUG     // Remove this symbol, if you would like to do debugging
#include <assert.h>
#define ASSERT(c)   assert(c)
//---------------------------------------------------------------------------
#define MeasureAccuracy FALSE
#if MeasureAccuracy
#include <stdio.h>
#endif
//---------------------------------------------------------------------------
#define PRINT_ORIGINAL_CHAR TRUE
#undef PRINT_ORIGINAL_CHAR
#define ADDITIONAL_BAUDRATE TRUE
#undef ADDITIONAL_BAUDRATE
//---------------------------------------------------------------------------
template <class T> T LASTP(T p){
        int l = strlen(p);
        if( l ) p += (l-1);
        return p;
};

// Diddle modes
enum {
        diddleNONE,
        diddleBLANKS,
        diddleLTTRS,
};

void __fastcall SetDirName(LPSTR t, LPCSTR pName);

//---------------------------------------------------------------------------

typedef short (_stdcall *dlReadPort)(short portaddr);
typedef void (_stdcall *dlWritePort)(short portaddr, short datum);

class CDLPort {
private:
        HANDLE                  m_hDLib;
        AnsiString              m_LastError;
        AnsiString              m_Path;

private:
        int __fastcall Open(void);
        void __fastcall Close(void);
        BOOL __fastcall IsFile(LPCSTR pName);

public:
        __fastcall CDLPort(LPCSTR pPath);
        __fastcall ~CDLPort();
        BOOL __fastcall IsOpen(void){return (m_hDLib!=NULL);};
};

typedef struct {        // Should not put a class into the member
        DWORD   m_dwVersion;
        int             m_WinNT;
        char    m_BgnDir[MAX_PATH];
        char    m_ModuleName[MAX_PATH];
        CDLPort *m_pDLPort;
}SYS;
extern SYS      sys;
//---------------------------------------------------------------------------
class CFSK {
private:
        volatile int    m_nFSK;
        volatile int    m_nPTT;

        volatile int    m_Sequence;
        volatile int    m_Count;
        volatile int    m_BCount;
        volatile int    m_NowD;

        volatile int    m_Idle;

        volatile int    m_invFSK;
        volatile int    m_oFSK;
        volatile int    m_aFSK;

        volatile int    m_invPTT;
        volatile int    m_oPTT;
        volatile int    m_aPTT;

        int m_shift_state;

#if MeasureAccuracy
        LARGE_INTEGER   m_liFreq;
        LARGE_INTEGER   m_liPCur, m_liPOld;
        DWORDLONG       m_dlDiff;
#endif
private:
        inline int __fastcall IsOpen(void){ return m_hPort != INVALID_HANDLE_VALUE;};

public:
        volatile HANDLE m_hPort;
        volatile int m_Baud;
        volatile int m_StgD;
        __fastcall CFSK(void);
        void __fastcall Init(void);
        void __fastcall Timer(void);
        inline void __fastcall Disable(void){m_hPort = INVALID_HANDLE_VALUE;};
        void __fastcall SetPTT(int sw, TMemo* Memo = NULL);
        inline int __fastcall IsBusy(void){
                return (m_StgD != -1) ? TRUE : FALSE;
        };
        inline void __fastcall SetInvFSK(int inv){
                m_invFSK = inv;
                m_aFSK = -1;
        };
        inline void __fastcall SetInvPTT(int inv){
                m_invPTT = inv;
                m_aPTT = -1;
        };
        inline void __fastcall SetDelay(int n){m_Count = n;};
#if MeasureAccuracy
        inline DWORDLONG __fastcall GetPDiff(void){return m_dlDiff;};
        inline DWORDLONG __fastcall GetPFreq(void){return m_liFreq.QuadPart;};
#endif
        BOOL __fastcall tinyIt( BYTE c, TMemo * Memo = NULL );
        BYTE __fastcall baudot2ascii( BYTE c );
        void __fastcall printWKstatus( TMemo * Memo );
        void ErrorExit(LPTSTR lpszFunction);
};
//---------------------------------------------------------------------------
class TExtFSK : public TForm
{
__published:    // IDE
        TMemo *Memo;
        TLabel *L1;
        TComboBox *PortName;
        TLabel *LComStat;
        TRadioGroup *RGDiddle;
        TCheckBox *CBInvFSK;
        TCheckBox *CBInvPTT;
        TSpeedButton *SBMin;
        TLabel *LabelBaud;
        void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
        void __fastcall PortNameChange(TObject *Sender);

        void __fastcall SBMinClick(TObject *Sender);
        void __fastcall CBInvFSKClick(TObject *Sender);
        void __fastcall CBInvPTTClick(TObject *Sender);
        void __fastcall RGDiddleClick(TObject *Sender);


        void __fastcall FormPaint(TObject *Sender);
        //void __fastcall RadioGroupSpeedClick(TObject *Sender);
private:
        int             m_WindowState;
        int             m_DisEvent;
        int             m_ptt;
        int             m_X;
        AnsiString      m_IniName;

        TIMECAPS        m_TimeCaps;
        HANDLE          m_hPort;
        DCB             m_dcb;
        CFSK            m_fsk;
        UINT            m_uTimerID;

        int             m_shift_state;
        int             m_baud;

        void __fastcall ReadIniFile(void);
        void __fastcall WriteIniFile(void);

        inline int __fastcall IsOpen(void){ return m_hPort != INVALID_HANDLE_VALUE;};
        void __fastcall UpdateComStat(void);
        void __fastcall UpdatePort();
        void __fastcall OpenPort(void);
        BOOL __fastcall OpenPort_(void);
        void __fastcall ClosePort(void);
        void __fastcall setInvFsk( bool b );

public:
        LONG m_para;  // Params passed from MMTTY
        __fastcall TExtFSK(TComponent* Owner);
        void __fastcall SetPara();
        void __fastcall SetPTT(int sw, int msg);
        void __fastcall PutChar(BYTE c);
        inline int __fastcall IsBusy(void){return m_fsk.IsBusy();};

};
//---------------------------------------------------------------------------
#endif

