;
;       Moon landing (MK14 Manual)
;
;       Keys 1-7 control the thrust
;
;       Major display problems on the old emulator
;

#define DatPtr(x)       ((x)+1)         ; useful SC/MP macros
#define High(x)         ((x)/256)
#define Low(x)          ((x) & 255)

Grav    = 5                             ; Gravity
Disp    = 0D00h                         ; I/O
Crom    = 010Bh                         ; Bin to 7 segment table
E       = 80h

Save    = 0F05h
H1      = 0f06h
L1      = 0f07h
Alt     = 0f08h
Vel     = 0f0Bh
Accn    = 0f0Eh
Thr     = 0f10h
Fule    = 0f12h

        .org    0F14h
Init:   .db     08h,50h,00h,99h,80h,00h ; Initial values
        .db     99h,98h,00h,02h,58h,00h

Ret:    xppc    2                       ; P2 contains 0F20h
        st      DatPtr(Save)            ; save value
        ldi     High(Crom)              ; P1 = CROM save it too
        xpah    1
        st      DatPtr(H1)
        ldi     Low(Crom)
        xpal    1
        st      DatPtr(L1)
        ld      DatPtr(Save)
        ccl
        ani     0Fh
        xae
Loop:   ld      E(1)
        st      @1(3)
        ldi     0
        dly     4
        ld      DatPtr(Save)
        sr
        sr
        sr
        sr
        xae
        csa
        scl
        jp      Loop
        ldi     0
        st      @1(3)
        ld      DatPtr(H1)
        xpah    1
        ld      DatPtr(L1)
        xpal    1
        jmp     Ret

Row     = 0F03h-Ret
Count   = 0F04h-Ret

;
;       Start here
;
Start:  ldi     High(Init)              ; P1 = Init
        xpah    1
        ldi     Low(Init)
        xpal    1
        ldi     High(Ret)               ; P2 = Display Function
        xpah    2
        ldi     Low(Ret)
        xpal    2
        ldi     12                      ; copy data to start
        st      Count(2)
Set:    ld      11(1)
        st      @-1(1)
        dld     Count(2)
        jnz     Set

Again:  ldi     High(Disp-1)            ; P3 = Display - 1
        xpah    3
        ldi     Low(Disp-1)
        xpal    3
        ldi     1
        st      Count(2)
        ld      @6(1)                   ; P1 -> Vel+2
        jp      Twice                   ; attitude positive ?
        ld      @4(1)                   ; P1 -> Thr+1
        jmp     Off                     ; don't update

Twice:  ldi     2                       ; Update velocity and then
        st      Row(2)
        ccl
Dadd:   ld      @-1(1)
        dad     2(1)
        st      (1)
        dld     Row(2)
        jnz     Dadd
        ld      2(1)
        jp      Pos                     ; gone negative ?
        ldi     99h
Pos:    dad     @-1(1)
        st      (1)
        dld     Count(2)
        jp      Twice
        ld      @12(1)                  ; P1 -> Alt
        ild     Row(2)
        scl
DSub:   ld      @-1(1)                  ; fuel
        cad     -2(1)                   ; - thrust
        st      (1)
        nop
        dld     Row(2)
        jp      DSub
        csa
        jp      Off
        jmp     Accns
Off:    ldi     0
        st      -1(1)                   ; zero thrust
Accns:  ld      -1(1)
        scl
        dai     099h-Grav
        st      -3(1)                   ; accn+1
        ldi     99h
        dai     0
        st      -4(1)                   ; accn
Dispy:  ld      (1)                     ; fuel
        xppc    2                       ; display it
        ld      -7(1)                   ; velocity
        jp      Posv
        ldi     99h
        scl
        cad     -6(1)                   ; velocity+1
        scl
        dai     0
        jmp     Sto
Posv:   ld      -6(1)                   ; velocity + 1
Sto:    xppc    2                       ; display velocity
        ld      -9(1)                   ; altitude + 1
        xppc    2                       ; display it
        ld      @-1(3)                  ; get rid of tank
        ld      @-10(1)                 ; P1 -> Alt now
        xppc    2
        ldi     10
        st      Count(2)
Toil:   ld      @-1(3)                  ; key pressed
        jp      Press
        xri     0DFh                    ; restart if command key
        jz      Start-Ret-1(2)
        dld     Count(2)
        jnz     Toil
        jmp     Again-Ret-1(2)
Press:  ld      9(1)                    ; Thr+1
        jz      Back                    ; stopped ?
        xpal    3                       ; which row
        st      9(1)                    ; set thrust
Back:   jmp     Again-Ret-1(2)

        .end

