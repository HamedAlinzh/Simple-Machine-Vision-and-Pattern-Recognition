; =============================================================
; فایل اسمبلی NASM برای محاسبه کانولوشن تصویر (SIMD)
; این کد روی لینوکس ۶۴ بیتی (Linux x64 ABI) اجرا می‌شود.
; =============================================================

section .text
global apply_convolution_asm

; پروتوتایپ تابع در سی به صورت زیر است:
; void apply_convolution_asm(float* input, float* output, int width, int height, float* kernel)
;
; طبق استاندارد لینوکس (System V AMD64 ABI)، پارامترها در رجیسترهای زیر قرار می‌گیرند:
; RDI = اشاره‌گر به آرایه ورودی (input pixels)
; RSI = اشاره‌گر به آرایه خروجی (output pixels)
; RDX = عرض تصویر (width)
; RCX = ارتفاع تصویر (height)
; R8  = اشاره‌گر به ماتریس فیلتر (kernel 3x3)

apply_convolution_asm:
    ; --- ذخیره رجیسترهای حساس ---
    ; طبق استاندارد، باید مقادیر این رجیسترها را حفظ کنیم و در پایان برگردانیم.
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; --- تنظیم حلقه‌ها ---
    ; ما یک حاشیه ۱ پیکسلی را نادیده می‌گیریم تا نیازی به چک کردن لبه‌های تصویر نباشد.
    ; حلقه Y از ۱ شروع می‌شود (سطر دوم)
    mov r10, 1              ; R10 شمارنده حلقه عمودی (y)

y_loop:
    ; بررسی شرط پایان حلقه Y: آیا y >= height - 1 ؟
    mov rax, rcx            ; کپی ارتفاع به RAX
    dec rax                 ; ارتفاع - ۱
    cmp r10, rax
    jge end_func            ; اگر به سطر آخر رسیدیم، پایان تابع

    ; حلقه X از ۱ شروع می‌شود (ستون دوم)
    mov r11, 1              ; R11 شمارنده حلقه افقی (x)

x_loop:
    ; بررسی شرط پایان حلقه X: آیا x >= width - 4 ؟
    ; ما ۴ پیکسل را همزمان پردازش می‌کنیم، پس باید ۴ پیکسل تا لبه فاصله داشته باشیم.
    mov rax, rdx            ; کپی عرض به RAX
    sub rax, 4              ; عرض - ۴
    cmp r11, rax
    jge next_line           ; اگر به انتهای خط رسیدیم، برو خط بعد

    ; =============================================================
    ; هسته اصلی پردازش (SIMD Convolution)
    ; هدف: محاسبه مقدار ۴ پیکسل خروجی به صورت همزمان
    ; فرمول: Output = Sum( Pixel[neighbor] * Kernel[neighbor] )
    ; =============================================================

    xorps xmm0, xmm0        ; پاک کردن XMM0 (این رجیستر مجموع ضرب‌ها را نگه می‌دارد)

    ; --- سطر اول کرنل (بالا) ---
    
    ; ۱. همسایه بالا-چپ (Top-Left)
    movss xmm1, [r8]        ; بارگذاری اولین عدد کرنل (K[0])
    shufps xmm1, xmm1, 0    ; کپی کردن K[0] در هر ۴ خانه XMM1 (پخش کردن)
    
    ; محاسبه آدرس حافظه برای همسایه بالا-چپ: (y-1)*width + (x-1)
    mov rax, r10            ; y
    dec rax                 ; y-1
    imul rax, rdx           ; (y-1) * width
    add rax, r11            ; + x
    dec rax                 ; + (x-1) (این شد اندیس پیکسل بالا چپ)
    shl rax, 2              ; ضرب در ۴ (چون هر float چهار بایت است)
    
    movups xmm2, [rdi + rax]; بارگذاری ۴ پیکسل همجوار از تصویر ورودی
    mulps xmm2, xmm1        ; ضرب موازی ۴ پیکسل در ضریب کرنل
    addps xmm0, xmm2        ; افزودن نتیجه به مجموع کل

    ; ۲. همسایه بالا-وسط (Top-Center)
    movss xmm1, [r8 + 4]    ; بارگذاری K[1] (۴ بایت جلوتر)
    shufps xmm1, xmm1, 0    ; پخش کردن
    movups xmm2, [rdi + rax + 4] ; بارگذاری پیکسل‌ها (یک خانه جلوتر از قبلی)
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; ۳. همسایه بالا-راست (Top-Right)
    movss xmm1, [r8 + 8]    ; بارگذاری K[2]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 8] ; دو خانه جلوتر
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; --- سطر دوم کرنل (وسط) ---
    
    ; محاسبه آدرس حافظه برای همسایه چپ: y*width + (x-1)
    mov rax, r10            ; y
    imul rax, rdx           ; y * width
    add rax, r11            ; + x
    dec rax                 ; + (x-1)
    shl rax, 2              ; ضرب در ۴ بایت

    ; ۴. همسایه چپ (Mid-Left)
    movss xmm1, [r8 + 12]   ; K[3]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; ۵. پیکسل مرکزی (Center)
    movss xmm1, [r8 + 16]   ; K[4]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 4]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; ۶. همسایه راست (Mid-Right)
    movss xmm1, [r8 + 20]   ; K[5]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 8]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; --- سطر سوم کرنل (پایین) ---

    ; محاسبه آدرس حافظه برای همسایه پایین-چپ: (y+1)*width + (x-1)
    mov rax, r10            ; y
    inc rax                 ; y+1
    imul rax, rdx           ; (y+1) * width
    add rax, r11            ; + x
    dec rax                 ; + (x-1)
    shl rax, 2              ; ضرب در ۴ بایت

    ; ۷. همسایه پایین-چپ (Bot-Left)
    movss xmm1, [r8 + 24]   ; K[6]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; ۸. همسایه پایین-وسط (Bot-Center)
    movss xmm1, [r8 + 28]   ; K[7]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 4]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; ۹. همسایه پایین-راست (Bot-Right)
    movss xmm1, [r8 + 32]   ; K[8]
    shufps xmm1, xmm1, 0
    movups xmm2, [rdi + rax + 8]
    mulps xmm2, xmm1
    addps xmm0, xmm2

    ; --- ذخیره نتیجه نهایی ---
    ; محاسبه آدرس خروجی: (y * width + x) * 4
    mov rax, r10            ; y
    imul rax, rdx           ; y * width
    add rax, r11            ; + x
    shl rax, 2              ; ضرب در ۴ بایت
    
    ; نوشتن ۴ پیکسل محاسبه شده از XMM0 به حافظه خروجی
    movups [rsi + rax], xmm0 

    ; افزایش شمارنده افقی (۴ پیکسل جلو می‌رویم)
    add r11, 4
    jmp x_loop              ; تکرار برای پیکسل‌های بعدی سطر

next_line:
    inc r10                 ; رفتن به سطر بعدی (y++)
    jmp y_loop              ; تکرار برای سطر بعدی

end_func:
    ; بازگرداندن رجیسترهای ذخیره شده و خروج
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret