section .text
global apply_convolution_asm

; Function Signature (Linux ABI):
; void apply_convolution_asm(float* input, float* output, int width, int height, float* kernel)
;
; Registers:
; RDI = input image pointer
; RSI = output image pointer
; RDX = width (int)
; RCX = height (int)
; R8  = kernel pointer

apply_convolution_asm:
    ; Prologue: Save registers we might modify (callee-saved)
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; ----------------------------------------------------------------
    ; Setup Loop Bounds
    ; We ignore the 1-pixel border to avoid boundary checks (simplification)
    ; y starts at 1, ends at height-1
    ; x starts at 1, ends at width-1
    ; ----------------------------------------------------------------
    
    ; R10 = y index (start at 1)
    mov r10, 1

y_loop:
    ; Check if y >= height - 1
    mov rax, rcx        ; rax = height
    dec rax             ; rax = height - 1
    cmp r10, rax
    jge end_func        ; If y >= height-1, we are done

    ; R11 = x index (start at 1)
    mov r11, 1

x_loop:
    ; Check if x >= width - 4 (Process 4 pixels at a time!)
    ; If fewer than 4 pixels remain, we skip them (simple safety for project)
    mov rax, rdx        ; rax = width
    sub rax, 4          ; Safety margin
    cmp r11, rax
    jge next_line       ; If x is too close to edge, move to next line

    ; ----------------------------------------------------------------
    ; Core Convolution Logic (The "Hot" Part)
    ; We want to calculate 4 output pixels: Out[x], Out[x+1], Out[x+2], Out[x+3]
    ; Initialize Sum Accumulator (XMM0) to ZERO
    ; ----------------------------------------------------------------
    xorps xmm0, xmm0    ; Clear XMM0 (Accumulator)

    ; --- Kernel Row 1 (Top) ---
    ; Neighbor 1 (Top-Left): K[0] * Pixels[x-1, y-1]
    ; We load one kernel value, broadcast it to all 4 slots of XMM1
    movss xmm1, [r8]        ; Load K[0] (scalar)
    shufps xmm1, xmm1, 0    ; Broadcast K[0] to {K0, K0, K0, K0}

    ; Calculate memory offset for input pixels at (x-1, y-1)
    ; Offset = ((y-1) * width + (x-1)) * 4 bytes
    mov rax, r10            ; y
    dec rax                 ; y-1
    imul rax, rdx           ; (y-1)*width
    add rax, r11            ; + x
    dec rax                 ; + (x-1)
    shl rax, 2              ; * 4 (float size)
    
    ; Load 4 adjacent pixels from input
    movups xmm2, [rdi + rax] ; Load 4 floats (unaligned)
    mulps xmm2, xmm1         ; Multiply 4 pixels by K[0]
    addps xmm0, xmm2         ; Add to accumulator

    ; Neighbor 2 (Top-Center): K[1] * Pixels[x, y-1]
    movss xmm1, [r8 + 4]     ; Load K[1]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 4] ; Shift input ptr by 1 float (4 bytes)
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; Neighbor 3 (Top-Right): K[2] * Pixels[x+1, y-1]
    movss xmm1, [r8 + 8]     ; Load K[2]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 8] 
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; --- Kernel Row 2 (Middle) ---
    ; Calculate offset for (x-1, y)
    ; Offset = (y * width + (x-1)) * 4 bytes
    mov rax, r10            ; y
    imul rax, rdx           ; y*width
    add rax, r11            ; + x
    dec rax                 ; + (x-1)
    shl rax, 2              ; * 4 bytes

    ; Neighbor 4 (Mid-Left): K[3]
    movss xmm1, [r8 + 12]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; Neighbor 5 (Center): K[4]
    movss xmm1, [r8 + 16]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 4]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; Neighbor 6 (Mid-Right): K[5]
    movss xmm1, [r8 + 20]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 8]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; --- Kernel Row 3 (Bottom) ---
    ; Calculate offset for (x-1, y+1)
    mov rax, r10            ; y
    inc rax                 ; y+1
    imul rax, rdx           ; (y+1)*width
    add rax, r11            ; + x
    dec rax                 ; + (x-1)
    shl rax, 2              ; * 4

    ; Neighbor 7 (Bot-Left): K[6]
    movss xmm1, [r8 + 24]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; Neighbor 8 (Bot-Center): K[7]
    movss xmm1, [r8 + 28]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 4]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; Neighbor 9 (Bot-Right): K[8]
    movss xmm1, [r8 + 32]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 8]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; ----------------------------------------------------------------
    ; Store Result
    ; Offset for Output = (y * width + x) * 4
    ; ----------------------------------------------------------------
    mov rax, r10            ; y
    imul rax, rdx           ; y*width
    add rax, r11            ; + x
    shl rax, 2              ; * 4
    
    movups [rsi + rax], xmm0 ; Store 4 calculated pixels to output

    ; Increment x by 4 (since we processed 4 pixels)
    add r11, 4
    jmp x_loop

next_line:
    inc r10                 ; y++
    jmp y_loop

end_func:
    ; Epilogue: Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret