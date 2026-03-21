; collision.asm - Fast collision detection for C64 Tetris
; 6502 assembly optimized for speed

.importzp ptr1, ptr2, ptr3
.importzp tmp1, tmp2, tmp3, tmp4
.export _check_collision_asm

; C-callable: uint8_t check_collision_asm(const GameState* game, int8_t x, int8_t y, uint8_t rotation);
; Returns 0 = no collision, 1 = collision

.segment "CODE"

.proc _check_collision_asm
    ; Save registers
    pha
    txa
    pha
    tya
    pha
    
    ; Set up pointers
    ; ptr1 = game pointer (passed in A/X)
    sta ptr1
    stx ptr1+1
    
    ; Get parameters from stack
    ; C calling convention: parameters pushed right-to-left
    ldy #9          ; Offset to rotation parameter
    lda (sp),y
    sta tmp4        ; tmp4 = rotation
    
    dey
    lda (sp),y
    sta tmp3        ; tmp3 = y (signed)
    
    dey
    lda (sp),y
    sta tmp2        ; tmp2 = x (signed)
    
    ; Calculate shape pointer
    ; shape = tetrominoes[current_piece][rotation]
    ldy #0
    lda (ptr1),y    ; Get current_piece from GameState
    sta tmp1
    
    ; Each piece is 4*4*4 = 64 bytes
    ; rotation * 16 (4x4 bytes)
    lda tmp4        ; rotation
    asl             ; *2
    asl             ; *4
    asl             ; *8
    asl             ; *16
    clc
    adc tmp1        ; Add piece offset (will be multiplied later)
    
    ; For now, simplified - we'll implement full lookup later
    ; Set ptr2 to point to shape data
    lda #<_tetromino_shapes
    sta ptr2
    lda #>_tetromino_shapes
    sta ptr2+1
    
    ; Main collision check loop
    ldx #0          ; y index
    ldy #0          ; x index
    
check_loop_y:
    cpx #4
    beq no_collision
    
    ; Get shape value
    ; ptr3 = ptr2 + rotation*16 + y*4 + x
    txa
    asl             ; *2
    asl             ; *4
    sta ptr3
    tya
    clc
    adc ptr3
    tay
    
    lda (ptr2),y
    beq next_cell   ; Skip if shape cell is empty
    
    ; Calculate board coordinates
    ; board_x = x + piece_x
    lda tmp2        ; piece_x
    clc
    adc ptr3+1      ; x coordinate (we need to store this somewhere)
    
    ; Check X bounds
    bmi collision   ; x < 0
    cmp #10         ; BOARD_WIDTH
    bcs collision   ; x >= 10
    
    ; Calculate board_y = y + piece_y
    lda tmp3        ; piece_y
    clc
    adc ptr3+2      ; y coordinate
    
    ; Check Y bounds
    bmi collision   ; y < 0 (above board)
    cmp #22         ; BOARD_HEIGHT
    bcs collision   ; y >= 22
    
    ; Check board collision
    ; Calculate board index = board_y * 10 + board_x
    ; Multiply by 10 (8+2)
    sta tmp1        ; Save board_y
    asl             ; *2
    asl             ; *4
    asl             ; *8
    clc
    adc tmp1        ; *9
    adc tmp1        ; *10
    clc
    adc ptr3+1      ; Add board_x
    
    ; ptr3 = game->board + index
    lda ptr1
    clc
    adc #<game_board_offset  ; Offset of board in GameState
    sta ptr3
    lda ptr1+1
    adc #>game_board_offset
    sta ptr3+1
    
    lda (ptr3),y
    bne collision   ; Board cell not empty
    
next_cell:
    iny
    cpy #4
    bne check_loop_y
    ldy #0
    inx
    jmp check_loop_y
    
collision:
    lda #1
    jmp done
    
no_collision:
    lda #0
    
done:
    ; Restore registers
    pla
    tay
    pla
    tax
    pla
    
    rts
.endproc

; Tetromino shape data (simplified - will be filled by C code)
.segment "RODATA"
_tetromino_shapes:
    ; I piece rotations
    .byte 0,0,0,0, 1,1,1,1, 0,0,0,0, 0,0,0,0
    .byte 0,0,1,0, 0,0,1,0, 0,0,1,0, 0,0,1,0
    .byte 0,0,0,0, 0,0,0,0, 1,1,1,1, 0,0,0,0
    .byte 0,1,0,0, 0,1,0,0, 0,1,0,0, 0,1,0,0
    
    ; Other pieces would follow...
    ; (Full implementation would include all 7 pieces * 4 rotations)