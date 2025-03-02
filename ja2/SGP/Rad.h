// This is not free software.
// This file contains code derived from the code released under the terms
// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.

#ifndef __RAD__
#define __RAD__

#define RADCOPYRIGHT "Copyright (C) 1994-98 RAD Game Tools, Inc."

#ifndef __RADRES__

//  __RADDOS__ means DOS code (16 or 32 bit)
//  __RAD16__ means 16 bit code (Win16)
//  __RAD32__ means 32 bit code (DOS, Win386, Win32s, Mac)
//  __RADWIN__ means Windows code (Win16, Win386, Win32s)
//  __RADWINEXT__ means Windows 386 extender (Win386)
//  __RADNT__ means Win32s code
//  __RADMAC__ means Macintosh
//  __RAD68K__ means 68K Macintosh
//  __RADPPC__ means PowerMac

#if (defined(__MWERKS__) && !defined(__INTEL__)) || defined(THINK_C) || defined(powerc) || \
    defined(macintosh) || defined(__powerc)

#define __RADMAC__
#if defined(powerc) || defined(__powerc)
#define __RADPPC__
#else
#define __RAD68K__
#endif

#define __RAD32__

#else

#ifdef __DOS__
#define __RADDOS__
#endif

#ifdef __386__
#define __RAD32__
#endif

#ifdef _Windows  // For Borland
#ifdef __WIN32__
#define WIN32
#else
#define __WINDOWS__
#endif
#endif

#ifdef _WINDOWS  // For MS
#ifndef _WIN32
#define __WINDOWS__
#endif
#endif

#ifdef _WIN32
#define __RADWIN__
#define __RADNT__
#define __RAD32__
#else
#ifdef __NT__
#define __RADWIN__
#define __RADNT__
#define __RAD32__
#else
#ifdef __WINDOWS_386__
#define __RADWIN__
#define __RADWINEXT__
#define __RAD32__
#else
#ifdef __WINDOWS__
#define __RADWIN__
#define __RAD16__
#else
#ifdef WIN32
#define __RADWIN__
#define __RADNT__
#define __RAD32__
#endif
#endif
#endif
#endif
#endif

#endif

#if (!defined(__RADDOS__) && !defined(__RADWIN__) && !defined(__RADMAC__))
#error RAD.H did not detect your platform.  Define __DOS__, __WINDOWS__, WIN32, macintosh, or powerc.
#endif

#ifdef __RADMAC__

// this define is for CodeWarrior 11's stupid new libs (even though
//   we don't use longlong's).

#define __MSL_LONGLONG_SUPPORT__

#define RADLINK
#define RADEXPLINK

#ifdef __CFM68K__
#ifdef __RADINDLL__
#define RADEXPFUNC RADDEFFUNC __declspec(export)
#else
#define RADEXPFUNC RADDEFFUNC __declspec(import)
#endif
#else
#define RADEXPFUNC RADDEFFUNC
#endif
#define RADASMLINK

#else

#ifdef __RADNT__
#ifndef _WIN32
#define _WIN32
#endif
#ifndef WIN32
#define WIN32
#endif
#endif

#ifdef __RADWIN__
#ifdef __RAD32__
#ifdef __RADNT__

#define RADLINK __stdcall
#define RADEXPLINK __stdcall

#ifdef __RADINEXE__
#define RADEXPFUNC RADDEFFUNC
#else
#ifndef __RADINDLL__
#define RADEXPFUNC RADDEFFUNC __declspec(dllimport)
#ifdef __BORLANDC__
#if __BORLANDC__ <= 0x460
#undef RADEXPFUNC
#define RADEXPFUNC RADDEFFUNC
#endif
#endif
#else
#define RADEXPFUNC RADDEFFUNC __declspec(dllexport)
#endif
#endif
#else
#define RADLINK __pascal
#define RADEXPLINK __far __pascal
#define RADEXPFUNC RADDEFFUNC
#endif
#else
#define RADLINK __pascal
#define RADEXPLINK __far __pascal __export
#define RADEXPFUNC RADDEFFUNC
#endif
#else
#define RADLINK __pascal
#define RADEXPLINK __pascal
#define RADEXPFUNC RADDEFFUNC
#endif

#define RADASMLINK __cdecl

#endif

#ifdef __RADWIN__
#ifndef _WINDOWS
#define _WINDOWS
#endif
#endif

#ifdef __cplusplus
#define RADDEFFUNC extern "C"
#define RADDEFSTART extern "C" {
#define RADDEFEND }
#else
#define RADDEFFUNC
#define RADDEFSTART
#define RADDEFEND
#endif

RADDEFSTART

#define s8 signed char
#define u8 unsigned char
#define u32 unsigned long
#define s32 signed long
#define u64 unsigned __int64
#define s64 signed __int64

#ifdef __RAD32__
#define PTR4

#define u16 unsigned short
#define s16 signed short

#ifdef __RADMAC__

#include <OSUtils.h>
#include <memory.h>
#include <string.h>

#define radstrlen strlen

#define radmemset memset

#define radmemcmp memcmp

#define radmemcpy(dest, source, size) BlockMoveData((Ptr)(source), (Ptr)(dest), size)

#define radmemcpydb(dest, source, size) BlockMoveData((Ptr)(source), (Ptr)(dest), size)

#define radstrcat strcat

#define radstrcpy strcpy

static u32 inline radsqr(s32 a) { return (a * a); }

#ifdef __RAD68K__

#pragma parameter __D0 mult64anddiv(__D0, __D1, __D2)
u32 mult64anddiv(u32 m1, u32 m2, u32 d) = {0x4C01, 0x0C01, 0x4C42, 0x0C01};
//  muls.l d1,d1:d0  divs.l d2,d1:d0

#pragma parameter radconv32a(__A0, __D0)
void radconv32a(void *p, u32 n) = {0x4A80, 0x600C, 0x2210, 0xE059, 0x4841,
                                   0xE059, 0x20C1, 0x5380, 0x6EF2};
// tst.l d0  bra.s @loope  @loop:  move.l (a0),d1  ror.w #8,d1  swap d1 ror.w
// #8,d1  move.l d1,(a0)+  sub.l #1,d0  bgt.s @loop  @loope:

#else

u32 mult64anddiv(u32 m1, u32 m2, u32 d);

void radconv32a(void *p, u32 n);

#endif

#else

#ifdef __WATCOMC__

u32 radsqr(s32 a);
#pragma aux radsqr = "mul eax" parm[eax] modify[EDX eax];

u32 mult64anddiv(u32 m1, u32 m2, u32 d);
#pragma aux mult64anddiv = \
    "mul ecx"              \
    "div ebx" parm[eax][ecx][ebx] modify[EDX eax];

s32 radabs(s32 ab);
#pragma aux radabs = \
    "test eax,eax"   \
    "jge skip"       \
    "neg eax"        \
    "skip:" parm[eax];

#define radabs32 radabs

u32 DOSOut(const char *str);
#pragma aux DOSOut =     \
    "cld"                \
    "mov ecx,0xffffffff" \
    "xor eax,eax"        \
    "mov edx,edi"        \
    "repne scasb"        \
    "not ecx"            \
    "dec ecx"            \
    "mov ebx,1"          \
    "mov ah,0x40"        \
    "int 0x21" parm[EDI] modify[EAX EBX ECX EDX EDI] value[ecx];

void DOSOutNum(const char *str, u32 len);
#pragma aux DOSOutNum = \
    "mov ah,0x40"       \
    "mov ebx,1"         \
    "int 0x21" parm[edx][ecx] modify[eax ebx];

u32 ErrOut(const char *str);
#pragma aux ErrOut =     \
    "cld"                \
    "mov ecx,0xffffffff" \
    "xor eax,eax"        \
    "mov edx,edi"        \
    "repne scasb"        \
    "not ecx"            \
    "dec ecx"            \
    "xor ebx,ebx"        \
    "mov ah,0x40"        \
    "int 0x21" parm[EDI] modify[EAX EBX ECX EDX EDI] value[ecx];

void ErrOutNum(const char *str, u32 len);
#pragma aux ErrOutNum = \
    "mov ah,0x40"       \
    "xor ebx,ebx"       \
    "int 0x21" parm[edx][ecx] modify[eax ebx];

void radmemset16(void *dest, u16 value, u32 size);
#pragma aux radmemset16 = \
    "cld"                 \
    "mov bx,ax"           \
    "shl eax,16"          \
    "mov ax,bx"           \
    "mov bl,cl"           \
    "shr ecx,1"           \
    "rep stosd"           \
    "mov cl,bl"           \
    "and cl,1"            \
    "rep stosw" parm[EDI][EAX][ECX] modify[EAX EDX EBX ECX EDI];

void radmemset(void *dest, u8 value, u32 size);
#pragma aux radmemset = \
    "cld"               \
    "mov ah,al"         \
    "mov bx,ax"         \
    "shl eax,16"        \
    "mov ax,bx"         \
    "mov bl,cl"         \
    "shr ecx,2"         \
    "and bl,3"          \
    "rep stosd"         \
    "mov cl,bl"         \
    "rep stosb" parm[EDI][AL][ECX] modify[EAX EDX EBX ECX EDI];

void radmemset32(void *dest, u32 value, u32 size);
#pragma aux radmemset32 = \
    "cld"                 \
    "rep stosd" parm[EDI][EAX][ECX] modify[EAX EDX EBX ECX EDI];

void radmemcpy(void *dest, const void *source, u32 size);
#pragma aux radmemcpy = \
    "cld"               \
    "mov bl,cl"         \
    "shr ecx,2"         \
    "rep movsd"         \
    "mov cl,bl"         \
    "and cl,3"          \
    "rep movsb" parm[EDI][ESI][ECX] modify[EBX ECX EDI ESI];

void __far *radfmemcpy(void __far *dest, const void __far *source, u32 size);
#pragma aux radfmemcpy = \
    "cld"                \
    "push es"            \
    "push ds"            \
    "mov es,cx"          \
    "mov ds,dx"          \
    "mov ecx,eax"        \
    "shr ecx,2"          \
    "rep movsd"          \
    "mov cl,al"          \
    "and cl,3"           \
    "rep movsb"          \
    "pop ds"             \
    "pop es" parm[CX EDI][DX ESI][EAX] modify[ECX EDI ESI] value[CX EDI];

void radmemcpydb(void *dest, const void *source, u32 size);  // Destination
                                                             // bigger
#pragma aux radmemcpydb = \
    "std"                 \
    "mov bl,cl"           \
    "lea esi,[esi+ecx-4]" \
    "lea edi,[edi+ecx-4]" \
    "shr ecx,2"           \
    "rep movsd"           \
    "and bl,3"            \
    "jz dne"              \
    "add esi,3"           \
    "add edi,3"           \
    "mov cl,bl"           \
    "rep movsb"           \
    "dne:"                \
    "cld" parm[EDI][ESI][ECX] modify[EBX ECX EDI ESI];

char *radstrcpy(void *dest, const void *source);
#pragma aux radstrcpy = \
    "cld"               \
    "mov edx,edi"       \
    "lp:"               \
    "mov al,[esi]"      \
    "inc esi"           \
    "mov [edi],al"      \
    "inc edi"           \
    "cmp al,0"          \
    "jne lp" parm[EDI][ESI] modify[EAX EDX EDI ESI] value[EDX];

char __far *radfstrcpy(void __far *dest, const void __far *source);
#pragma aux radfstrcpy = \
    "cld"                \
    "push es"            \
    "push ds"            \
    "mov es,cx"          \
    "mov ds,dx"          \
    "mov edx,edi"        \
    "lp:"                \
    "lodsb"              \
    "stosb"              \
    "test al,0xff"       \
    "jnz lp"             \
    "pop ds"             \
    "pop es" parm[CX EDI][DX ESI] modify[EAX EDX EDI ESI] value[CX EDX];

char *radstpcpy(void *dest, const void *source);
#pragma aux radstpcpy = \
    "cld"               \
    "lp:"               \
    "mov al,[esi]"      \
    "inc esi"           \
    "mov [edi],al"      \
    "inc edi"           \
    "cmp al,0"          \
    "jne lp"            \
    "dec edi" parm[EDI][ESI] modify[EAX EDI ESI] value[EDI];

char *radstpcpyrs(void *dest, const void *source);
#pragma aux radstpcpyrs = \
    "cld"                 \
    "lp:"                 \
    "mov al,[esi]"        \
    "inc esi"             \
    "mov [edi],al"        \
    "inc edi"             \
    "cmp al,0"            \
    "jne lp"              \
    "dec esi" parm[EDI][ESI] modify[EAX EDI ESI] value[ESI];

u32 radstrlen(const void *dest);
#pragma aux radstrlen =  \
    "cld"                \
    "mov ecx,0xffffffff" \
    "xor eax,eax"        \
    "repne scasb"        \
    "not ecx"            \
    "dec ecx" parm[EDI] modify[EAX ECX EDI] value[ECX];

char *radstrcat(void *dest, const void *source);
#pragma aux radstrcat =  \
    "cld"                \
    "mov ecx,0xffffffff" \
    "mov edx,edi"        \
    "xor eax,eax"        \
    "repne scasb"        \
    "dec edi"            \
    "lp:"                \
    "lodsb"              \
    "stosb"              \
    "test al,0xff"       \
    "jnz lp" parm[EDI][ESI] modify[EAX ECX EDI ESI] value[EDX];

char *radstrchr(const void *dest, char chr);
#pragma aux radstrchr = \
    "cld"               \
    "lp:"               \
    "lodsb"             \
    "cmp al,dl"         \
    "je fnd"            \
    "cmp al,0"          \
    "jnz lp"            \
    "mov esi,1"         \
    "fnd:"              \
    "dec esi" parm[ESI][DL] modify[EAX ESI] value[esi];

s8 radmemcmp(const void *s1, const void *s2, u32 len);
#pragma aux radmemcmp = \
    "cld"               \
    "rep cmpsb"         \
    "setne al"          \
    "jbe end"           \
    "neg al"            \
    "end:" parm[EDI][ESI][ECX] modify[ECX EDI ESI];

s8 radstrcmp(const void *s1, const void *s2);
#pragma aux radstrcmp = \
    "lp:"               \
    "mov al,[esi]"      \
    "mov ah,[edi]"      \
    "cmp al,ah"         \
    "jne set"           \
    "cmp al,0"          \
    "je set"            \
    "inc esi"           \
    "inc edi"           \
    "jmp lp"            \
    "set:"              \
    "setne al"          \
    "jbe end"           \
    "neg al"            \
    "end:" parm[EDI][ESI] modify[EAX EDI ESI];

s8 radstricmp(const void *s1, const void *s2);
#pragma aux radstricmp = \
    "lp:"                \
    "mov al,[esi]"       \
    "mov ah,[edi]"       \
    "cmp al,'a'"         \
    "jb c1"              \
    "cmp al,'z'"         \
    "ja c1"              \
    "sub al,32"          \
    "c1:"                \
    "cmp ah,'a'"         \
    "jb c2"              \
    "cmp ah,'z'"         \
    "ja c2"              \
    "sub ah,32"          \
    "c2:"                \
    "cmp al,ah"          \
    "jne set"            \
    "cmp al,0"           \
    "je set"             \
    "inc esi"            \
    "inc edi"            \
    "jmp lp"             \
    "set:"               \
    "setne al"           \
    "jbe end"            \
    "neg al"             \
    "end:" parm[EDI][ESI] modify[EAX EDI ESI];

s8 radstrnicmp(const void *s1, const void *s2, u32 len);
#pragma aux radstrnicmp = \
    "lp:"                 \
    "mov al,[esi]"        \
    "mov ah,[edi]"        \
    "cmp al,'a'"          \
    "jb c1"               \
    "cmp al,'z'"          \
    "ja c1"               \
    "sub al,32"           \
    "c1:"                 \
    "cmp ah,'a'"          \
    "jb c2"               \
    "cmp ah,'z'"          \
    "ja c2"               \
    "sub ah,32"           \
    "c2:"                 \
    "cmp al,ah"           \
    "jne set"             \
    "cmp al,0"            \
    "je set"              \
    "dec ecx"             \
    "jz set"              \
    "inc esi"             \
    "inc edi"             \
    "jmp lp"              \
    "set:"                \
    "setne al"            \
    "jbe end"             \
    "neg al"              \
    "end:" parm[EDI][ESI][ECX] modify[EAX ECX EDI ESI];

char *radstrupr(void *s1);
#pragma aux radstrupr = \
    "mov ecx,edi"       \
    "lp:"               \
    "mov al,[edi]"      \
    "cmp al,'a'"        \
    "jb c1"             \
    "cmp al,'z'"        \
    "ja c1"             \
    "sub [edi],32"      \
    "c1:"               \
    "inc edi"           \
    "cmp al,0"          \
    "jne lp" parm[EDI] modify[EAX EDI] value[ecx];

char *radstrlwr(void *s1);
#pragma aux radstrlwr = \
    "mov ecx,edi"       \
    "lp:"               \
    "mov al,[edi]"      \
    "cmp al,'A'"        \
    "jb c1"             \
    "cmp al,'Z'"        \
    "ja c1"             \
    "add [edi],32"      \
    "c1:"               \
    "inc edi"           \
    "cmp al,0"          \
    "jne lp" parm[EDI] modify[EAX EDI] value[ecx];

u32 radstru32(const void *dest);
#pragma aux radstru32 = \
    "cld"               \
    "xor ecx,ecx"       \
    "xor ebx,ebx"       \
    "xor edi,edi"       \
    "lodsb"             \
    "cmp al,45"         \
    "jne skip2"         \
    "mov edi,1"         \
    "jmp skip"          \
    "lp:"               \
    "mov eax,10"        \
    "mul ecx"           \
    "lea ecx,[eax+ebx]" \
    "skip:"             \
    "lodsb"             \
    "skip2:"            \
    "cmp al,0x39"       \
    "ja dne"            \
    "cmp al,0x30"       \
    "jb dne"            \
    "mov bl,al"         \
    "sub bl,0x30"       \
    "jmp lp"            \
    "dne:"              \
    "test edi,1"        \
    "jz pos"            \
    "neg ecx"           \
    "pos:" parm[ESI] modify[EAX EBX EDX EDI ESI] value[ecx];

u16 GetDS();
#pragma aux GetDS = "mov ax,ds" value[ax];

#ifdef __RADWINEXT__

#define _16To32(ptr16)                                                              \
  ((void *)(((GetSelectorBase((u16)(((u32)(ptr16)) >> 16)) + ((u16)(u32)(ptr16))) - \
             GetSelectorBase(GetDS()))))

#endif

#ifndef __RADWIN__
#define int86 int386
#define int86x int386x
#endif

#define u32regs x
#define u16regs w

#else

#define radstrcpy strcpy
#define radstrcat strcat
#define radmemcpy memcpy
#define radmemcpydb memmove
#define radmemcmp memcmp
#define radmemset memset
#define radstrlen strlen
#define radstrchr strchr
#define radtoupper toupper
#define radstru32(s) ((u32)atol(s))
#define radstricmp _stricmp
#define radstrcmp strcmp
#define radstrupr _strupr
#define radstrlwr _strlwr
#define BreakPoint() _asm {int 3}

#ifdef _MSC_VER

#pragma warning(disable : 4035)

typedef char *RADPCHAR;

u32 __inline radsqr(u32 m) {
  _asm {
            mov eax,[m]
            mul eax
  }
}

u32 __inline mult64anddiv(u32 m1, u32 m2, u32 d) {
  _asm {
            mov eax,[m1]
            mov ecx,[m2]
            mul ecx
            mov ecx,[d]
            div ecx
  }
}

s32 __inline radabs(s32 ab) {
  _asm {
            mov eax,[ab]
            test eax,eax
            jge skip
            neg eax
           skip:
  }
}

u8 __inline radinp(u16 p) {
  _asm {
            mov dx,[p]
            in al,dx
  }
}

void __inline radoutp(u16 p, u8 v) {
  _asm {
            mov dx,[p]
            mov al,[v]
            out dx,al
  }
}

RADPCHAR __inline radstpcpy(char *p1, char *p2) {
  _asm {
             mov edx,[p1]
             mov ecx,[p2]
             cld
            lp:
             mov al,[ecx]
             inc ecx
             mov [edx],al
             inc edx
             cmp al,0
             jne lp
             dec edx
             mov eax,edx
  }
}

RADPCHAR __inline radstpcpyrs(char *p1, char *p2) {
  _asm {
            mov edx,[p1]
            mov ecx,[p2]
            cld
           lp:
            mov al,[ecx]
            inc ecx
            mov [edx],al
            inc edx
            cmp al,0
            jne lp
            dec ecx
            mov eax,ecx
  }
}

void __inline radmemset16(void *dest, u16 value, u32 sizeb) {
  _asm {
            mov edi,[dest]
            mov ax,[value]
            mov ecx,[sizeb]
            shl eax,16
            cld
            mov ax,[value]
            mov bl,cl
            shr ecx,1
            rep stosd
            mov cl,bl
            and cl,1
            rep stosw
  }
}

void __inline radmemset32(void *dest, u32 value, u32 sizeb) {
  _asm {
            mov edi,[dest]
            mov eax,[value]
            mov ecx,[sizeb]
            cld
            rep stosd
  }
}

u32 __inline __stdcall RADsqrt(u32 sq) {
  _asm {
            fild dword ptr [sq]
            fsqrt
            fistp word ptr [sq]
            movzx eax,word ptr [sq]
  }
}

void __inline RADCycleTimerStartAddr(u32 *addr) {
  _asm {
            mov ecx,[addr]
            __asm __emit 0fh __asm __emit 031h
            mov [ecx],eax
  }
}

u32 __inline RADCycleTimerDeltaAddr(u32 *addr) {
  _asm {
            __asm __emit 0fh __asm __emit 031h
            mov ecx,[addr]
            sub eax,[ecx]
  }
}

#define RADCycleTimerStart(var) RADCycleTimerStartAddr(&var)
#define RADCycleTimerDelta(var) RADCycleTimerDeltaAddr(&var)

#pragma warning(default : 4035)

#endif

#endif

#endif

#else

#define PTR4 __far

#define u16 unsigned int
#define s16 signed int

#ifdef __WATCOMC__

u32 radsqr(s32 a);
#pragma aux radsqr = \
    "shl edx,16"     \
    "mov dx,ax"      \
    "mov eax,edx"    \
    "xor edx,edx"    \
    "mul eax"        \
    "shld edx,eax,16" parm[dx ax] modify[DX ax] value[dx ax];

s16 radabs(s16 ab);
#pragma aux radabs = \
    "test ax,ax"     \
    "jge skip"       \
    "neg ax"         \
    "skip:" parm[ax] value[ax];

s32 radabs32(s32 ab);
#pragma aux radabs32 = \
    "test dx,dx"       \
    "jge skip"         \
    "neg dx"           \
    "neg ax"           \
    "sbb dx,0"         \
    "skip:" parm[dx ax] value[dx ax];

u32 DOSOut(const char far *dest);
#pragma aux DOSOut =                                                           \
    "cld"                                                                      \
    "and edi,0xffff"                                                           \
    "mov dx,di"                                                                \
    "mov ecx,0xffffffff"                                                       \
    "xor eax,eax" 0x67 "repne scasb"                                           \
                       "not ecx"                                               \
                       "dec ecx"                                               \
                       "mov bx,1"                                              \
                       "push ds"                                               \
                       "push es"                                               \
                       "pop ds"                                                \
                       "mov ah,0x40"                                           \
                       "int 0x21"                                              \
                       "pop ds"                                                \
                       "movzx eax,cx"                                          \
                       "shr ecx,16" parm[ES DI] modify[AX BX CX DX DI          \
                                                           ES] value[CX AX];

void DOSOutNum(const char far *str, u16 len);
#pragma aux DOSOutNum = \
    "push ds"           \
    "mov ds,cx"         \
    "mov cx,bx"         \
    "mov ah,0x40"       \
    "mov bx,1"          \
    "int 0x21"          \
    "pop ds" parm[cx dx][bx] modify[ax bx cx];

u32 ErrOut(const char far *dest);
#pragma aux ErrOut =                                                           \
    "cld"                                                                      \
    "and edi,0xffff"                                                           \
    "mov dx,di"                                                                \
    "mov ecx,0xffffffff"                                                       \
    "xor eax,eax" 0x67 "repne scasb"                                           \
                       "not ecx"                                               \
                       "dec ecx"                                               \
                       "xor bx,bx"                                             \
                       "push ds"                                               \
                       "push es"                                               \
                       "pop ds"                                                \
                       "mov ah,0x40"                                           \
                       "int 0x21"                                              \
                       "pop ds"                                                \
                       "movzx eax,cx"                                          \
                       "shr ecx,16" parm[ES DI] modify[AX BX CX DX DI          \
                                                           ES] value[CX AX];

void ErrOutNum(const char far *str, u16 len);
#pragma aux ErrOutNum = \
    "push ds"           \
    "mov ds,cx"         \
    "mov cx,bx"         \
    "mov ah,0x40"       \
    "xor bx,bx"         \
    "int 0x21"          \
    "pop ds" parm[cx dx][bx] modify[ax bx cx];

void radmemset(void far *dest, u8 value, u32 size);
#pragma aux radmemset = "cld"                                                  \
                        "and edi,0ffffh"                                       \
                        "shl ecx,16"                                           \
                        "mov cx,bx"                                            \
                        "mov ah,al"                                            \
                        "mov bx,ax"                                            \
                        "shl eax,16"                                           \
                        "mov ax,bx"                                            \
                        "mov bl,cl"                                            \
                        "shr ecx,2" 0x67 "rep stosd"                           \
                                         "mov cl,bl"                           \
                                         "and cl,3"                            \
                                         "rep stosb" parm[ES DI][AL][CX BX];

void radmemset16(void far *dest, u16 value, u32 size);
#pragma aux radmemset16 = \
    "cld"                 \
    "and edi,0ffffh"      \
    "shl ecx,16"          \
    "mov cx,bx"           \
    "mov bx,ax"           \
    "shl eax,16"          \
    "mov ax,bx"           \
    "mov bl,cl"           \
    "shr ecx,1"           \
    "rep stosd"           \
    "mov cl,bl"           \
    "and cl,1"            \
    "rep stosw" parm[ES DI][AX][CX BX];

void radmemcpy(void far *dest, const void far *source, u32 size);
#pragma aux radmemcpy =                                                        \
    "cld"                                                                      \
    "push ds"                                                                  \
    "mov ds,dx"                                                                \
    "and esi,0ffffh"                                                           \
    "and edi,0ffffh"                                                           \
    "shl ecx,16"                                                               \
    "mov cx,bx"                                                                \
    "shr ecx,2" 0x67 "rep movsd"                                               \
                     "mov cl,bl"                                               \
                     "and cl,3"                                                \
                     "rep movsb"                                               \
                     "pop ds" parm[ES DI][DX SI][CX BX] modify[CX SI DI ES];

s8 radmemcmp(const void far *s1, const void far *s2, u32 len);
#pragma aux radmemcmp = \
    "cld"               \
    "push ds"           \
    "mov ds,dx"         \
    "shl ecx,16"        \
    "mov cx,bx"         \
    "rep cmpsb"         \
    "setne al"          \
    "jbe end"           \
    "neg al"            \
    "end:"              \
    "pop ds" parm[ES DI][DX SI][CX BX] modify[CX SI DI ES];

char far *radstrcpy(void far *dest, const void far *source);
#pragma aux radstrcpy = \
    "cld"               \
    "push ds"           \
    "mov ds,dx"         \
    "and esi,0xffff"    \
    "and edi,0xffff"    \
    "mov dx,di"         \
    "lp:"               \
    "lodsb"             \
    "stosb"             \
    "test al,0xff"      \
    "jnz lp"            \
    "pop ds" parm[ES DI][DX SI] modify[AX DX DI SI ES] value[es dx];

char far *radstpcpy(void far *dest, const void far *source);
#pragma aux radstpcpy = \
    "cld"               \
    "push ds"           \
    "mov ds,dx"         \
    "and esi,0xffff"    \
    "and edi,0xffff"    \
    "lp:"               \
    "lodsb"             \
    "stosb"             \
    "test al,0xff"      \
    "jnz lp"            \
    "dec di"            \
    "pop ds" parm[ES DI][DX SI] modify[DI SI ES] value[es di];

u32 radstrlen(const void far *dest);
#pragma aux radstrlen =                                                        \
    "cld"                                                                      \
    "and edi,0xffff"                                                           \
    "mov ecx,0xffffffff"                                                       \
    "xor eax,eax" 0x67 "repne scasb"                                           \
                       "not ecx"                                               \
                       "dec ecx"                                               \
                       "movzx eax,cx"                                          \
                       "shr ecx,16" parm[ES DI] modify[AX CX DI                \
                                                           ES] value[CX AX];

char far *radstrcat(void far *dest, const void far *source);
#pragma aux radstrcat = "cld"                                                  \
                        "and edi,0xffff"                                       \
                        "mov ecx,0xffffffff"                                   \
                        "and esi,0xffff"                                       \
                        "push ds"                                              \
                        "mov ds,dx"                                            \
                        "mov dx,di"                                            \
                        "xor eax,eax" 0x67 "repne scasb"                       \
                                           "dec edi"                           \
                                           "lp:"                               \
                                           "lodsb"                             \
                                           "stosb"                             \
                                           "test al,0xff"                      \
                                           "jnz lp"                            \
                                           "pop ds" parm[ES DI]                \
    [DX SI] modify[AX CX DI SI ES] value[es dx];

char far *radstrchr(const void far *dest, char chr);
#pragma aux radstrchr =                                                        \
    "cld"                                                                      \
    "lp:" 0x26 "lodsb"                                                         \
               "cmp al,dl"                                                     \
               "je fnd"                                                        \
               "cmp al,0"                                                      \
               "jnz lp"                                                        \
               "xor ax,ax"                                                     \
               "mov es,ax"                                                     \
               "mov si,1"                                                      \
               "fnd:"                                                          \
               "dec si" parm[ES SI][DL] modify[AX SI ES] value[es si];

s8 radstricmp(const void far *s1, const void far *s2);
#pragma aux radstricmp = \
    "and edi,0xffff"     \
    "push ds"            \
    "mov ds,dx"          \
    "and esi,0xffff"     \
    "lp:"                \
    "mov al,[esi]"       \
    "mov ah,[edi]"       \
    "cmp al,'a'"         \
    "jb c1"              \
    "cmp al,'z'"         \
    "ja c1"              \
    "sub al,32"          \
    "c1:"                \
    "cmp ah,'a'"         \
    "jb c2"              \
    "cmp ah,'z'"         \
    "ja c2"              \
    "sub ah,32"          \
    "c2:"                \
    "cmp al,ah"          \
    "jne set"            \
    "cmp al,0"           \
    "je set"             \
    "inc esi"            \
    "inc edi"            \
    "jmp lp"             \
    "set:"               \
    "setne al"           \
    "jbe end"            \
    "neg al"             \
    "end:"               \
    "pop ds" parm[ES DI][DX SI] modify[AX DI SI];

u32 radstru32(const void far *dest);
#pragma aux radstru32 = "cld"                                                  \
                        "xor ecx,ecx"                                          \
                        "xor ebx,ebx"                                          \
                        "xor edi,edi" 0x26 "lodsb"                             \
                                           "cmp al,45"                         \
                                           "jne skip2"                         \
                                           "mov edi,1"                         \
                                           "jmp skip"                          \
                                           "lp:"                               \
                                           "mov eax,10"                        \
                                           "mul ecx"                           \
                                           "lea ecx,[eax+ebx]"                 \
                                           "skip:" 0x26 "lodsb"                \
                                                        "skip2:"               \
                                                        "cmp al,0x39"          \
                                                        "ja dne"               \
                                                        "cmp al,0x30"          \
                                                        "jb dne"               \
                                                        "mov bl,al"            \
                                                        "sub bl,0x30"          \
                                                        "jmp lp"               \
                                                        "dne:"                 \
                                                        "test edi,1"           \
                                                        "jz pos"               \
                                                        "neg ecx"              \
                                                        "pos:"                 \
                                                        "movzx eax,cx"         \
                                                        "shr ecx,16" parm      \
    [ES SI] modify[AX BX DX DI SI] value[cx ax];

u32 mult64anddiv(u32 m1, u32 m2, u32 d);
#pragma aux mult64anddiv = \
    "shl ecx,16"           \
    "mov cx,ax"            \
    "shrd eax,edx,16"      \
    "mov ax,si"            \
    "mul ecx"              \
    "shl edi,16"           \
    "mov di,bx"            \
    "div edi"              \
    "shld edx,eax,16"      \
    "and edx,0xffff"       \
    "and eax,0xffff" parm[cx ax][dx si][di bx] modify[ax bx cx dx si di] value[dx ax];

#endif

#endif

RADDEFEND

#define u32neg1 ((u32)(s32)-1)
#define RAD_align(var) \
  var;                 \
  u8 junk##var[4 - (sizeof(var) & 3)];
#define RAD_align_after(var) u8 junk##var[4 - (sizeof(var) & 3)] = {0};
#define RAD_align_init(var, val) \
  var = val;                     \
  u8 junk##var[4 - (sizeof(var) & 3)] = {0};
#define RAD_align_array(var, num) \
  var[num];                       \
  u8 junk##var[4 - (sizeof(var) & 3)];
#define RAD_align_string(var, str) \
  char var[] = str;                \
  u8 junk##var[4 - (sizeof(var) & 3)] = {0};

RADEXPFUNC void PTR4 *RADEXPLINK radmalloc(u32 numbytes);
RADEXPFUNC void RADEXPLINK radfree(void PTR4 *ptr);

#ifdef __WATCOMC__

char bkbhit();
#pragma aux bkbhit = \
    "mov ah,1"       \
    "int 0x16"       \
    "lahf"           \
    "shr eax,14"     \
    "and eax,1"      \
    "xor al,1";

char bgetch();
#pragma aux bgetch = \
    "xor ah,ah"      \
    "int 0x16"       \
    "test al,0xff"   \
    "jnz done"       \
    "mov al,ah"      \
    "or al,0x80"     \
    "done:" modify[AX];

void BreakPoint();
#pragma aux BreakPoint = "int 3";

u8 radinp(u16 p);
#pragma aux radinp = "in al,dx" parm[DX];

u8 radtoupper(u8 p);
#pragma aux radtoupper = \
    "cmp al,'a'"         \
    "jb c1"              \
    "cmp al,'z'"         \
    "ja c1"              \
    "sub al,32"          \
    "c1:" parm[al] value[al];

void radoutp(u16 p, u8 v);
#pragma aux radoutp = "out dx,al" parm[DX][AL];

#endif

// for multi-processor machines

#ifdef __RADNT__
#define LockedIncrement(var) _asm { lock inc [var]}
#define LockedDecrement(var) _asm { lock dec [var]}
#else
#define LockedIncrement(var) _asm { inc [var]}
#define LockedDecrement(var) _asm { dec [var]}
#endif

#endif

#endif
