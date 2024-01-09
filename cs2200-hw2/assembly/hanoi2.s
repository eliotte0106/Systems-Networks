!============================================================
! CS 2200 Homework 2 Part 2: Tower of Hanoi
!
! Apart from initializing the stack,
! please do not edit mains functionality. You do not need to
! save the return address before jumping to hanoi in
! main.
!============================================================

main:
    lea     $sp, stack              ! TODO: Here, you need to get the address of the stack
    lea     $fp, stack              ! using the provided label to initialize the stack pointer.
    lw      $sp, 0($sp)             ! load the label address into $sp and in the next instruction,      
    lw      $fp, 0($fp)             ! use $sp as base register to load the value (0xFFFF) into $sp.           

    lea     $at, hanoi              ! loads address of hanoi label into $at

    lea     $a0, testNumDisks2      ! loads address of number into $a0
    lw      $a0, 0($a0)             ! loads value of number into $a0

    jalr    $ra, $at                ! jump to hanoi, set $ra to return addr
    halt                            ! when we return, just halt

hanoi:
    addi    $sp, $sp, -1            ! TODO: perform post-call portion of
    sw      $fp, 0($sp)             ! the calling convention. Make sure to
    add    $fp, $sp, $zero          ! save any registers you will be using!

    addi    $t0, $zero, 1           ! TODO: Implement the following pseudocode in assembly:
    blt     $t0, $a0, else          ! IF ($a0 == 1)
    br      base                    !    GOTO base
                                    ! ELSE
                                    !    GOTO else

else:
    addi    $a0, $a0, -1            !TODO: perform recursion after decrementing
    lea     $at, hanoi              ! the parameter by 1. Remember, $a0 holds the
    addi    $sp, $sp, -1
    addi    $sp, $sp, -1               ! parameter value.           
    sw      $ra, -1($fp)
    sw      $a0, -2($fp)
    jalr    $ra, $at
    lw      $ra, -1($fp)
    addi    $sp, $sp, 1
    addi    $sp, $sp, 1

    add     $v0, $v0, $v0           ! TODO: Implement the following pseudocode in assembly:
    addi    $v0, $v0, 1             ! $v0 = 2 * $v0 + 1
    br      teardown                ! RETURN $v0

base:
    addi    $v0, $v0, 1             ! TODO: Return 1

teardown:
    lw      $fp, 0($fp)             ! TODO: perform pre-return portion
    addi    $sp, $sp, 1           ! of the calling convention
    jalr    $zero, $ra              ! return to caller



stack: .word 0xFFFF                 ! the stack begins here


! Words for testing \/

! 1
testNumDisks1:
    .word 0x0001

! 10
testNumDisks2:
    .word 0x000a

! 20
testNumDisks3:
    .word 0x0014
