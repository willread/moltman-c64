; moltball_asm.s — Performance-critical routines in 6502 assembly
; Assembled with ca65, linked with moltball.c via cl65
;
; CRITICAL: Only use cc65 scratch ZP (ptr1-ptr4, tmp1-tmp4, sreg).
; NEVER touch $02-$03 (sp) or $06-$09 (regsave) or $16-$1B (regbank).

.include "zeropage.inc"

.import _cur_px, _cur_py, _cur_i
.import _bdx, _bdy, _pfr, _walls_building, _wall_hit_idx
.import _spr_x_lo, _spr_x_hi, _spr_y
.import _fv_row, _scr_row

.export _move_one, _flush_sprites, _fill_visual_row

.segment "CODE"

; =====================================================================
; move_one — Move current ball 1 pixel with collision detection
;
; ZP allocation (all cc65 scratch — safe):
;   ptr3    ($0E-$0F) = npx (lo/hi)
;   tmp3    ($14)      = npy
;   tmp1    ($12)      = cl (left grid col)
;   tmp2    ($13)      = cr (right grid col)
;   tmp4    ($15)      = ct / scratch
;   ptr4    ($10-$11)  = cb (lo byte only) / scratch
;   ptr1    ($0A-$0B)  = row pointer (top)
;   ptr2    ($0C-$0D)  = row pointer (bottom)
;
; Matches C move_one_and_wall exactly:
;   - Wall check: bounces on PF_WALL_A(2)/PF_WALL_B(3) only
;   - Solid check: bounces on PF_SOLID(1) only — NOT any non-zero!
;   - X bounce uses (npx, cur_py)
;   - Y bounce uses (cur_px, npy)
; =====================================================================
.proc _move_one
    ; --- npx = cur_px + bdx[cur_i] ---
    ldx     _cur_i
    lda     _bdx,x
    bmi     @dx_neg
    ; dx = +1
    clc
    lda     _cur_px
    adc     #1
    sta     ptr3
    lda     _cur_px+1
    adc     #0
    sta     ptr3+1
    jmp     @npx_done
@dx_neg:
    ; dx = -1
    sec
    lda     _cur_px
    sbc     #1
    sta     ptr3
    lda     _cur_px+1
    sbc     #0
    sta     ptr3+1
@npx_done:

    ; --- npy = cur_py + bdy[cur_i] ---
    ldx     _cur_i
    lda     _cur_py
    clc
    adc     _bdy,x
    sta     tmp3            ; npy

    ; --- cl = SHR3(npx + BHB_L) ---
    ;     BHB_L = 6
    clc
    lda     ptr3
    adc     #6
    sta     tmp4            ; temp lo
    lda     ptr3+1
    adc     #0              ; hi byte with carry
    asl
    asl
    asl
    asl
    asl                     ; hi << 5
    sta     tmp1            ; store partial
    lda     tmp4
    lsr
    lsr
    lsr                     ; lo >> 3
    ora     tmp1
    sta     tmp1            ; cl

    ; --- cr = SHR3(npx + BHB_R) ---
    ;     BHB_R = 17
    clc
    lda     ptr3
    adc     #17
    sta     tmp4
    lda     ptr3+1
    adc     #0
    asl
    asl
    asl
    asl
    asl
    sta     tmp2
    lda     tmp4
    lsr
    lsr
    lsr
    ora     tmp2
    sta     tmp2            ; cr

    ; ========== WALL CHECK ==========
    lda     _walls_building
    bne     @do_wall
    jmp     @no_wall
@do_wall:
    ; ct = (npy + 4) >> 3
    lda     tmp3
    clc
    adc     #4
    lsr
    lsr
    lsr
    sta     tmp4            ; ct

    ; cb = (npy + 15) >> 3
    lda     tmp3
    clc
    adc     #15
    lsr
    lsr
    lsr
    sta     ptr4            ; cb

    ; ptr1 = pfr[ct]
    lda     tmp4
    asl
    tax
    lda     _pfr,x
    sta     ptr1
    lda     _pfr+1,x
    sta     ptr1+1

    ; ptr2 = pfr[cb]
    lda     ptr4
    asl
    tax
    lda     _pfr,x
    sta     ptr2
    lda     _pfr+1,x
    sta     ptr2+1

    ; Check all 4 corners for >= PF_WALL_A (2)
    ldy     tmp1            ; cl
    lda     (ptr1),y
    cmp     #2
    bcs     @wall_hit

    ldy     tmp2            ; cr
    lda     (ptr1),y
    cmp     #2
    bcs     @wall_hit

    ldy     tmp1
    lda     (ptr2),y
    cmp     #2
    bcs     @wall_hit

    ldy     tmp2
    lda     (ptr2),y
    cmp     #2
    bcs     @wall_hit

    jmp     @no_wall

@wall_hit:
    ; A = cell value (2 or 3). wall index = value - 2
    sec
    sbc     #2
    sta     _wall_hit_idx
    ; Accept new position (ball passes through)
    lda     ptr3
    sta     _cur_px
    lda     ptr3+1
    sta     _cur_px+1
    lda     tmp3
    sta     _cur_py
    rts

@no_wall:
    lda     #$FF
    sta     _wall_hit_idx

    ; ========== X BOUNCE ==========
    ; Uses (npx for X, cur_py for Y)
    ; cl/cr already computed from npx

    ; ct = (cur_py + 4) >> 3
    lda     _cur_py
    clc
    adc     #4
    lsr
    lsr
    lsr
    sta     tmp4            ; ct

    ; cb = (cur_py + 15) >> 3
    lda     _cur_py
    clc
    adc     #15
    lsr
    lsr
    lsr
    sta     ptr4            ; cb

    ; --- Left boundary: (npx + 6) <= 16 ---
    ; npx + 6 is already computed for cl, but let's redo cleanly
    ; Actually: if hi byte of (npx+6) != 0, definitely > 16
    clc
    lda     ptr3
    adc     #6
    sta     sreg            ; temp lo
    lda     ptr3+1
    adc     #0
    bne     @x_not_left     ; hi != 0 → > 255 → not left boundary
    lda     sreg
    cmp     #17             ; < 17 means <= 16
    bcc     @x_bounce
@x_not_left:

    ; --- Right boundary: (npx + 17) >= 303 ($012F) ---
    clc
    lda     ptr3
    adc     #17
    sta     sreg            ; temp lo
    lda     ptr3+1
    adc     #0
    cmp     #1
    bcc     @x_not_right    ; hi < 1 → < 256 → not right
    bne     @x_bounce       ; hi > 1 → >= 512 → definitely right
    ; hi == 1
    lda     sreg
    cmp     #$2F            ; lo >= $2F → >= 303
    bcs     @x_bounce
@x_not_right:

    ; --- Grid check: PF_SOLID (1) ONLY ---
    ; ptr1 = pfr[ct]
    lda     tmp4
    asl
    tax
    lda     _pfr,x
    sta     ptr1
    lda     _pfr+1,x
    sta     ptr1+1

    ; ptr2 = pfr[cb]
    lda     ptr4
    asl
    tax
    lda     _pfr,x
    sta     ptr2
    lda     _pfr+1,x
    sta     ptr2+1

    ldy     tmp1            ; cl
    lda     (ptr1),y
    cmp     #1
    beq     @x_bounce

    ldy     tmp2            ; cr
    lda     (ptr1),y
    cmp     #1
    beq     @x_bounce

    ldy     tmp1
    lda     (ptr2),y
    cmp     #1
    beq     @x_bounce

    ldy     tmp2
    lda     (ptr2),y
    cmp     #1
    beq     @x_bounce

    ; No X collision — accept npx
    lda     ptr3
    sta     _cur_px
    lda     ptr3+1
    sta     _cur_px+1
    jmp     @y_check

@x_bounce:
    ldx     _cur_i
    sec
    lda     #0
    sbc     _bdx,x
    sta     _bdx,x
    ; fall through to Y check

@y_check:
    ; ========== Y BOUNCE ==========
    ; Uses (cur_px for X, npy for Y)
    ; Must recompute cl/cr from cur_px (may have changed if X accepted)

    ; cl = SHR3(cur_px + 6)
    clc
    lda     _cur_px
    adc     #6
    sta     tmp4
    lda     _cur_px+1
    adc     #0
    asl
    asl
    asl
    asl
    asl
    sta     tmp1
    lda     tmp4
    lsr
    lsr
    lsr
    ora     tmp1
    sta     tmp1            ; new cl

    ; cr = SHR3(cur_px + 17)
    clc
    lda     _cur_px
    adc     #17
    sta     tmp4
    lda     _cur_px+1
    adc     #0
    asl
    asl
    asl
    asl
    asl
    sta     tmp2
    lda     tmp4
    lsr
    lsr
    lsr
    ora     tmp2
    sta     tmp2            ; new cr

    ; ct = (npy + 4) >> 3
    lda     tmp3
    clc
    adc     #4
    lsr
    lsr
    lsr
    sta     tmp4            ; ct

    ; cb = (npy + 15) >> 3
    lda     tmp3
    clc
    adc     #15
    lsr
    lsr
    lsr
    sta     ptr4            ; cb

    ; --- Top boundary: (npy + 4) <= 24 ---
    lda     tmp3
    clc
    adc     #4
    cmp     #25             ; < 25 means <= 24
    bcc     @y_bounce

    ; --- Bottom boundary: (npy + 15) >= 175 ---
    lda     tmp3
    clc
    adc     #15
    cmp     #175
    bcs     @y_bounce

    ; --- Grid check: PF_SOLID (1) ONLY ---
    lda     tmp4
    asl
    tax
    lda     _pfr,x
    sta     ptr1
    lda     _pfr+1,x
    sta     ptr1+1

    lda     ptr4
    asl
    tax
    lda     _pfr,x
    sta     ptr2
    lda     _pfr+1,x
    sta     ptr2+1

    ldy     tmp1
    lda     (ptr1),y
    cmp     #1
    beq     @y_bounce

    ldy     tmp2
    lda     (ptr1),y
    cmp     #1
    beq     @y_bounce

    ldy     tmp1
    lda     (ptr2),y
    cmp     #1
    beq     @y_bounce

    ldy     tmp2
    lda     (ptr2),y
    cmp     #1
    beq     @y_bounce

    ; No Y collision — accept npy
    lda     tmp3
    sta     _cur_py
    rts

@y_bounce:
    ldx     _cur_i
    sec
    lda     #0
    sbc     _bdy,x
    sta     _bdy,x
    rts
.endproc


; =====================================================================
; flush_sprites — Write all 8 sprite positions to VIC-II registers
; Uses only A, X, Y registers (no ZP)
; =====================================================================
.proc _flush_sprites
    ldy     #$00
    ldx     #$00
@loop:
    lda     _spr_x_lo,x
    sta     $D000,y
    lda     _spr_y,x
    sta     $D001,y
    inx
    iny
    iny
    cpx     #$08
    bne     @loop

    lda     #$00
    ldx     #$07
@msb:
    asl     a
    ora     _spr_x_hi,x
    dex
    bpl     @msb
    sta     $D010
    rts
.endproc


; =====================================================================
; fill_visual_row — Process one row of visual fill
; Uses ptr1 (pfr row), ptr2 (screen row), ptr3 (colram row)
; =====================================================================
.proc _fill_visual_row
    lda     _fv_row
    asl
    tax

    ; ptr1 = pfr[fv_row]
    lda     _pfr,x
    sta     ptr1
    lda     _pfr+1,x
    sta     ptr1+1

    ; ptr2 = SCREEN_BASE + scr_row[fv_row]
    lda     _scr_row,x
    sta     ptr2
    sta     ptr3            ; same low byte for colram
    lda     _scr_row+1,x
    clc
    adc     #$4c            ; SCREEN_BASE high byte
    sta     ptr2+1

    lda     _scr_row+1,x
    clc
    adc     #$d8            ; COLRAM_BASE high byte
    sta     ptr3+1

    ldy     #2              ; PF_X
@scan:
    lda     (ptr1),y        ; pfr[row][x]
    cmp     #1              ; PF_SOLID?
    bne     @skip
    lda     (ptr2),y        ; screen char
    cmp     #128            ; CH_TILE?
    bne     @skip
    lda     #32             ; CH_FILLED (space)
    sta     (ptr2),y
    lda     #0              ; COL_BG (black)
    sta     (ptr3),y
@skip:
    iny
    cpy     #38             ; PF_X + PF_W
    bne     @scan
    rts
.endproc

; =====================================================================
; flood_fill_run — Flood fill marking PF_OPEN cells as PF_MARK
;
; Uses fsx[]/fsy[] as stack, fsp as 16-bit stack pointer
; pfr[] is array of 25 row pointers
;
; PF_OPEN = 0, PF_MARK = 4
; =====================================================================

.import _fsx, _fsy, _fsp, _pfr

.export _flood_fill_run

PF_OPEN_V = 0
PF_MARK_V = 4

_flood_fill_run:
    ; Main loop: while fsp > 0
@loop:
    lda _fsp
    ora _fsp+1
    bne @not_done
    jmp @done
@not_done:

    ; --fsp
    lda _fsp
    bne @nodec_hi
    dec _fsp+1
@nodec_hi:
    dec _fsp
    
    ; Load index into X (lo byte) and Y (hi byte for page calc)
    ; fsx and fsy are accessed at [fsp]
    lda _fsp
    sta ptr1        ; index lo
    lda _fsp+1
    sta ptr1+1      ; index hi

    ; fx = fsx[fsp] — load via pointer arithmetic
    clc
    lda #<_fsx
    adc ptr1
    sta ptr2
    lda #>_fsx
    adc ptr1+1
    sta ptr2+1
    ldy #0
    lda (ptr2),y
    sta tmp1        ; tmp1 = fx

    ; fy = fsy[fsp]
    clc
    lda #<_fsy
    adc ptr1
    sta ptr2
    lda #>_fsy
    adc ptr1+1
    sta ptr2+1
    lda (ptr2),y
    sta tmp2        ; tmp2 = fy

    ; row = pfr[fy] — load row pointer
    lda tmp2
    asl a           ; fy * 2 (pointer is 2 bytes)
    tay
    lda _pfr,y
    sta ptr3        ; ptr3 = row pointer lo
    lda _pfr+1,y
    sta ptr3+1      ; ptr3 = row pointer hi

    ; Check RIGHT: row[fx+1]
    lda tmp1
    clc
    adc #1
    tay
    lda (ptr3),y    ; row[fx+1]
    cmp #PF_OPEN_V
    bne @skip_right
    ; Mark it
    lda #PF_MARK_V
    sta (ptr3),y
    ; Push fx+1, fy
    jsr @push_right
@skip_right:

    ; Check LEFT: row[fx-1]
    lda tmp1
    sec
    sbc #1
    tay
    lda (ptr3),y    ; row[fx-1]
    cmp #PF_OPEN_V
    bne @skip_left
    lda #PF_MARK_V
    sta (ptr3),y
    jsr @push_left
@skip_left:

    ; Check DOWN: pfr[fy+1][fx]
    lda tmp2
    clc
    adc #1
    asl a
    tay
    lda _pfr,y
    sta ptr4
    lda _pfr+1,y
    sta ptr4+1
    ldy tmp1
    lda (ptr4),y
    cmp #PF_OPEN_V
    bne @skip_down
    lda #PF_MARK_V
    sta (ptr4),y
    ; Push fx, fy+1
    lda tmp2
    clc
    adc #1
    sta tmp3        ; fy+1
    jsr @push_down
@skip_down:

    ; Check UP: pfr[fy-1][fx]
    lda tmp2
    sec
    sbc #1
    asl a
    tay
    lda _pfr,y
    sta ptr4
    lda _pfr+1,y
    sta ptr4+1
    ldy tmp1
    lda (ptr4),y
    cmp #PF_OPEN_V
    bne @skip_up
    lda #PF_MARK_V
    sta (ptr4),y
    lda tmp2
    sec
    sbc #1
    sta tmp3        ; fy-1
    jsr @push_up
@skip_up:

    jmp @loop

@done:
    rts

; --- Capacity check macro (shared by all push paths) ---
; Returns (falls through) if OK, jumps to @cap_full if full
.macro CHECK_CAP
    lda _fsp+1
    cmp #>(FILL_STACK_SZ)
    bcc :+          ; hi < hi_limit → OK
    bne @cap_full   ; hi > hi_limit → full
    lda _fsp
    cmp #<(FILL_STACK_SZ)
    bcs @cap_full   ; lo >= lo_limit → full
:
.endmacro

; --- Push: tmp4=x, A=y → fsx[fsp], fsy[fsp], ++fsp ---
@push:
    pha             ; save y
    clc
    lda #<_fsx
    adc _fsp
    sta ptr2
    lda #>_fsx
    adc _fsp+1
    sta ptr2+1
    lda tmp4
    ldy #0
    sta (ptr2),y
    clc
    lda #<_fsy
    adc _fsp
    sta ptr2
    lda #>_fsy
    adc _fsp+1
    sta ptr2+1
    pla
    sta (ptr2),y
    inc _fsp
    bne @push_done
    inc _fsp+1
@push_done:
    rts

@cap_full:
    rts

@push_right:
    CHECK_CAP
    lda tmp1
    clc
    adc #1
    sta tmp4
    lda tmp2
    jmp @push

@push_left:
    CHECK_CAP
    lda tmp1
    sec
    sbc #1
    sta tmp4
    lda tmp2
    jmp @push

@push_down:
    CHECK_CAP
    lda tmp1
    sta tmp4
    lda tmp3
    jmp @push

@push_up:
    CHECK_CAP
    lda tmp1
    sta tmp4
    lda tmp3
    jmp @push

FILL_STACK_SZ = 700
