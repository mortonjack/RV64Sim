        .text
        
        .globl  _start

        .org    0x00000000
trap_vector_reset:      
        
_start:
        j       reset_vector

        .org    0x00000080
trap_vector_exception:
        j       exception_handler

        .org    0x0000008c
trap_vector_machine_software_int:
        j       trap_vector_reset

        .org    0x00000090
trap_vector_user_timer_int:
        j       trap_vector_reset

        .org    0x0000009c
trap_vector_machine_timer_int:
        j       trap_vector_reset

        .org    0x000000a0
trap_vector_user_external_int:
        j       trap_vector_reset

        .org    0x000000ac
trap_vector_machine_external_int:
        j       trap_vector_reset

        .org    0x00000100
        
reset_vector:   
        la      t0, trap_vector_exception
        ori     t0, t0, 1               # Vectored trap mode
        csrw    mtvec, t0
        .option push
        .option norelax
        la      gp, __global_pointer$   # Initialize gp
        .option pop
        li      sp, 0x00020000          # Initialize sp
        li      s0, 0                   # Initialize fp
        call    main
        j       trap_vector_reset

exception_handler:
        csrw    mscratch, t0            # Save trapped code's t0
        csrr    t0, mcause
        bltz    t0, trap_vector_reset   # User software interrupt, same vector as exceptions
        slli    t0, t0, 2
        jr      %lo(exception_table)(t0)
        
exception_table:
        j       exception_handler_instr_addr_misaligned
        j       exception_handler_not_implemented        # instr_access_fault
        j       exception_handler_illegal_instr
        j       exception_handler_breakpoint
        j       exception_handler_load_addr_misaligned
        j       exception_handler_not_implemented       # load_access_fault
        j       exception_handler_store_addr_misaligned
        j       exception_handler_not_implemented       # store_access_fault
        j       exception_handler_user_env_call
        j       exception_handler_not_implemented       # supervisor_env_call
        j       exception_handler_not_implemented       # reserved
        j       exception_handler_machine_env_call

exception_handler_not_implemented:
        j       trap_vector_reset
        
exception_handler_instr_addr_misaligned:
        j       trap_vector_reset

exception_handler_illegal_instr:
        j       trap_vector_reset
        
exception_handler_breakpoint:
        j       trap_vector_reset
        
exception_handler_load_addr_misaligned:
        j       step_to_next_instr
        
exception_handler_store_addr_misaligned:
        j       step_to_next_instr
        
exception_handler_user_env_call:     
        j       trap_vector_reset

exception_handler_machine_env_call:     
        j       trap_vector_reset


step_to_next_instr:
        csrr    t0, mepc
        addi    t0, t0, 4
        csrw    mepc, t0
        csrr    t0, mscratch
        mret

        
        .globl  clear_misaligned_trapped
clear_misaligned_trapped:
        la      a0, misaligned_trapped
        sw      x0, 0(a0)
        ret

        .globl  get_misaligned_trapped
get_misaligned_trapped:
        la      a0, misaligned_trapped
        lw      a0, 0(a0)
        ret

        .data

misaligned_trapped:
        .word   0

        
