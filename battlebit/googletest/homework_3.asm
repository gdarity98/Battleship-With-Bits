        global    find_max
        section   .text
find_max:
        ;; rdi has the array in it
        ;; rsi has the length in it
        mov rax, [rdi] ;; move the first value in the array into rax
        jmp to_loop

to_loop:
        cmp [rdi], rax
        jg cont_loop
        add rdi,8
        sub rsi, 1
        cmp rsi, 0
        jne to_loop
        je all_done

        ;; you will need to loop through the array and
        ;; compare each value with rax to determine if it is greater
        ;; after the comparison, decrement the count, bump the
        ;; array pointer by 8 (long long = 64 bits = 8 bytes)
        ;; and if the counter is greater than zero, loop

cont_loop:
        mov rax, [rdi]
        add rdi,8
        sub rsi, 1
        cmp rsi, 0
        jne to_loop
        je all_done

all_done:
        ret