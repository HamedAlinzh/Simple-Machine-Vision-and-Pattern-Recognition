global calculate_distance_simd
section .text

; float calculate_distance_simd(float* img1, float* img2);
; rdi = pointer to img1 (test image)
; rsi = pointer to img2 (template image)
calculate_distance_simd:
    vxorps ymm0, ymm0, ymm0    ; ymm0 = 0 (Accumulator for the sum)
    mov rcx, 98                ; Loop counter: 784 pixels / 8 floats per YMM = 98 iterations

.loop:
    vmovups ymm1, [rdi]        ; Load 8 floats from img1
    vmovups ymm2, [rsi]        ; Load 8 floats from img2
    
    vsubps ymm1, ymm1, ymm2    ; ymm1 = img1 - img2
    vmulps ymm1, ymm1, ymm1    ; ymm1 = (img1 - img2)^2
    
    vaddps ymm0, ymm0, ymm1    ; Accumulate the sum in ymm0
    
    add rdi, 32                ; Move img1 pointer by 32 bytes (8 floats * 4 bytes)
    add rsi, 32                ; Move img2 pointer by 32 bytes
    dec rcx
    jnz .loop

    ; Horizontal add to sum the 8 floats inside ymm0 into a single float
    vextractf128 xmm1, ymm0, 1 
    vaddps xmm0, xmm0, xmm1
    vhaddps xmm0, xmm0, xmm0
    vhaddps xmm0, xmm0, xmm0   ; The final sum is now in the lowest 32 bits of xmm0

    ret                        ; Return the float value in xmm0 (standard C calling convention)