# Faulty oops analysis:

## OUTPUT FROM ./runqemu.sh
#  echo “hello_world” > /dev/faulty
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
Mem abort info:
  ESR = 0x96000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=00000000420c6000
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 96000045 [#1] SMP
Modules linked in: scull(O) faulty(O) hello(O)
CPU: 0 PID: 151 Comm: sh Tainted: G           O      5.15.18 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : faulty_write+0x14/0x20 [faulty]
lr : vfs_write+0xa8/0x2a0
sp : ffffffc008c83d80
x29: ffffffc008c83d80 x28: ffffff80020e1980 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040001000 x22: 0000000000000012 x21: 000000558cb6aa00
x20: 000000558cb6aa00 x19: ffffff80020dfe00 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc0006f5000 x3 : ffffffc008c83df0
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000
Call trace:
 faulty_write+0x14/0x20 [faulty]
 ksys_write+0x68/0x100
 __arm64_sys_write+0x20/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0x100
 do_el0_svc+0x44/0xb0
 el0_svc+0x28/0x80
 el0t_64_sync_handler+0xa4/0x130
 el0t_64_sync+0x1a0/0x1a4
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace 96c6c35525206797 ]---


##Analysis of oops message:
The oops messages are generated when we try to derefernce a NULL pointer or when we try to access an errous pointer value.
The first line of output - Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
tells us we are trying to access a inaccessible address space i.e. we are tryuing to deference a NULL Pointer
which has lead to this message.

In Linux, all addresses are are virtual mapped to physical addresss space via page tables. It results into page fault when
the mechanism fails to map the pointer to a location in the physical address space. The call trace's first line - faulty_write+0x14/0x20 [faulty] 
tells us that the faulty_write() function call has lead to this message. It is of 0x20 bytes size while the error
has occured due to instruction at 0x14 th byte 

# The faulty.c - faulty_write()
ssize_t faulty_write (struct file *filp, const char __user *buf, size_t count,
                loff_t *pos)
{
        /* make a simple fault by dereferencing a NULL pointer */
        *(int *)0 = 0;
        return 0;
}

This shows we are trying to write to a NULL pointer

# Objectdump:

faulty.ko:     file format elf64-littleaarch64


Disassembly of section .text:

0000000000000000 <faulty_write>:
   0:   d503245f        bti     c
   4:   d2800001        mov     x1, #0x0                        // #0
   8:   d2800000        mov     x0, #0x0                        // #0
   c:   d503233f        paciasp
  10:   d50323bf        autiasp
  14:   b900003f        str     wzr, Äx1Å
  18:   d65f03c0        ret
  1c:   d503201f        nop

0000000000000020 <faulty_read>:
  20:   d503233f        paciasp
  24:   a9bd7bfd        stp     x29, x30, Äsp, #-48Å!
  28:   d5384100        mrs     x0, sp_el0
  2c:   910003fd        mov     x29, sp
  30:   a90153f3        stp     x19, x20, Äsp, #16Å
  34:   aa0103f4        mov     x20, x1
  38:   aa0203f3        mov     x19, x2
  3c:   f941f801        ldr     x1, Äx0, #1008Å
  40:   f90017e1        str     x1, Äsp, #40Å
  44:   d2800001        mov     x1, #0x0                        // #0
  48:   d2800282        mov     x2, #0x14                       // #20
  4c:   52801fe1        mov     w1, #0xff                       // #255
  50:   910093e0        add     x0, sp, #0x24
  54:   94000000        bl      0 <memset>
  58:   d5384100        mrs     x0, sp_el0
  5c:   b9402401        ldr     w1, Äx0, #36Å
  60:   f100127f        cmp     x19, #0x4
  64:   d2800082        mov     x2, #0x4                        // #4
  68:   9a829273        csel    x19, x19, x2, ls  // ls = plast
  6c:   37a80361        tbnz    w1, #21, d8 <faulty_read+0xb8>
  70:   f9400000        ldr     x0, Äx0Å
  74:   aa1403e2        mov     x2, x20
  78:   7206001f        tst     w0, #0x4000000
  7c:   540002e1        b.ne    d8 <faulty_read+0xb8>  // b.any
  80:   b2409be1        mov     x1, #0x7fffffffff               // #549755813887
  84:   aa0103e0        mov     x0, x1
  88:   ab130042        adds    x2, x2, x19
  8c:   9a8083e0        csel    x0, xzr, x0, hi  // hi = pmore
  90:   da9f3042        csinv   x2, x2, xzr, cc  // cc = lo, ul, last
  94:   fa00005f        sbcs    xzr, x2, x0
  98:   9a9f87e2        cset    x2, ls  // ls = plast
  9c:   aa1303e0        mov     x0, x19
  a0:   b5000222        cbnz    x2, e4 <faulty_read+0xc4>
  a4:   7100001f        cmp     w0, #0x0
  a8:   d5384101        mrs     x1, sp_el0
  ac:   93407c00        sxtw    x0, w0
  b0:   9a931000        csel    x0, x0, x19, ne  // ne = any
  b4:   f94017e3        ldr     x3, Äsp, #40Å
  b8:   f941f822        ldr     x2, Äx1, #1008Å
  bc:   eb020063        subs    x3, x3, x2
  c0:   d2800002        mov     x2, #0x0                        // #0
  c4:   54000221        b.ne    108 <faulty_read+0xe8>  // b.any
  c8:   a94153f3        ldp     x19, x20, Äsp, #16Å
  cc:   a8c37bfd        ldp     x29, x30, ÄspÅ, #48
  d0:   d50323bf        autiasp
  d4:   d65f03c0        ret
  d8:   9340de82        sbfx    x2, x20, #0, #56
  dc:   8a020282        and     x2, x20, x2
  e0:   17ffffe8        b       80 <faulty_read+0x60>
  e4:   9340de82        sbfx    x2, x20, #0, #56
  e8:   8a020282        and     x2, x20, x2
  ec:   ea21005f        bics    xzr, x2, x1
  f0:   9a9f0280        csel    x0, x20, xzr, eq  // eq = none
  f4:   d503229f        csdb
  f8:   910093e1        add     x1, sp, #0x24
  fc:   aa1303e2        mov     x2, x19
 100:   94000000        bl      0 <__arch_copy_to_user>
 104:   17ffffe8        b       a4 <faulty_read+0x84>
 108:   94000000        bl      0 <__stack_chk_fail>
 10c:   d503201f        nop

0000000000000110 <faulty_init>:
 110:   d503233f        paciasp
 114:   a9be7bfd        stp     x29, x30, Äsp, #-32Å!
 118:   90000004        adrp    x4, 0 <faulty_write>
 11c:   910003fd        mov     x29, sp
 120:   f9000bf3        str     x19, Äsp, #16Å
 124:   90000013        adrp    x19, 0 <faulty_write>
 128:   b9400260        ldr     w0, Äx19Å
 12c:   90000003        adrp    x3, 0 <faulty_write>
 130:   91000084        add     x4, x4, #0x0
 134:   91000063        add     x3, x3, #0x0
 138:   52802002        mov     w2, #0x100                      // #256
 13c:   52800001        mov     w1, #0x0                        // #0
 140:   94000000        bl      0 <__register_chrdev>
 144:   37f800a0        tbnz    w0, #31, 158 <faulty_init+0x48>
 148:   b9400261        ldr     w1, Äx19Å
 14c:   350000e1        cbnz    w1, 168 <faulty_init+0x58>
 150:   b9000260        str     w0, Äx19Å
 154:   52800000        mov     w0, #0x0                        // #0
 158:   f9400bf3        ldr     x19, Äsp, #16Å
 15c:   a8c27bfd        ldp     x29, x30, ÄspÅ, #32
 160:   d50323bf        autiasp
 164:   d65f03c0        ret
 168:   52800000        mov     w0, #0x0                        // #0
 16c:   f9400bf3        ldr     x19, Äsp, #16Å
 170:   a8c27bfd        ldp     x29, x30, ÄspÅ, #32
 174:   d50323bf        autiasp
 178:   d65f03c0        ret
 17c:   d503201f        nop

0000000000000180 <cleanup_module>:
 180:   d503233f        paciasp
 184:   90000000        adrp    x0, 0 <faulty_write>
 188:   a9bf7bfd        stp     x29, x30, Äsp, #-16Å!
 18c:   52802002        mov     w2, #0x100                      // #256
 190:   52800001        mov     w1, #0x0                        // #0
 194:   910003fd        mov     x29, sp
 198:   b9400000        ldr     w0, Äx0Å
 19c:   90000003        adrp    x3, 0 <faulty_write>
 1a0:   91000063        add     x3, x3, #0x0
 1a4:   94000000        bl      0 <__unregister_chrdev>
 1a8:   a8c17bfd        ldp     x29, x30, ÄspÅ, #16
 1ac:   d50323bf        autiasp
 1b0:   d65f03c0        ret

Disassembly of section .rodata.str1.8:

0000000000000000 <.rodata.str1.8>:
   0:   6c756166        ldnp    d6, d24, Äx11, #-176Å
   4:   Address 0x0000000000000004 is out of bounds.


Disassembly of section .modinfo:

0000000000000000 <__UNIQUE_ID_license192>:
   0:   6563696c        fnmls   z12.h, p2/m, z11.h, z3.h
   4:   3d65736e        ldr     b14, Äx27, #2396Å
   8:   6c617544        ldnp    d4, d29, Äx10, #-496Å
   c:   44534220        smlalb  z0.h, z17.b, z19.b
  10:   4c50472f        .inst   0x4c50472f ; undefined
        ...

0000000000000015 <__UNIQUE_ID_depends194>:
  15:   65706564        fnmls   z4.h, p1/m, z11.h, z16.h
  19:   3d73646e        ldr     b14, Äx3, #3289Å
        ...

000000000000001e <__UNIQUE_ID_name193>:
  1e:   656d616e        fnmls   z14.h, p0/m, z11.h, z13.h
  22:   7561663d        .inst   0x7561663d ; undefined
  26:   0079746c        .inst   0x0079746c ; undefined

000000000000002a <__UNIQUE_ID_vermagic192>:
  2a:   6d726576        ldp     d22, d25, Äx11, #-224Å
  2e:   63696761        .inst   0x63696761 ; undefined
  32:   312e353d        adds    w29, w9, #0xb8d
  36:   38312e35        .inst   0x38312e35 ; undefined
  3a:   504d5320        adr     x0, 9aaa0 <cleanup_module+0x9a920>
  3e:   646f6d20        .inst   0x646f6d20 ; undefined
  42:   6c6e755f        ldnp    d31, d29, Äx10, #-288Å
  46:   2064616f        .inst   0x2064616f ; undefined
  4a:   63726161        .inst   0x63726161 ; undefined
  4e:   00343668        .inst   0x00343668 ; NYI

Disassembly of section .note.gnu.property:

0000000000000000 <.note.gnu.property>:
   0:   00000004        udf     #4
   4:   00000010        udf     #16
   8:   00000005        udf     #5
   c:   00554e47        .inst   0x00554e47 ; undefined
  10:   c0000000        .inst   0xc0000000 ; undefined
  14:   00000004        udf     #4
  18:   00000003        udf     #3
  1c:   00000000        udf     #0

Disassembly of section .note.gnu.build-id:

0000000000000000 <.note.gnu.build-id>:
   0:   00000004        udf     #4
   4:   00000014        udf     #20
   8:   00000003        udf     #3
   c:   00554e47        .inst   0x00554e47 ; undefined
  10:   4f66863f        .inst   0x4f66863f ; undefined
  14:   f7379cf6        .inst   0xf7379cf6 ; undefined
  18:   a691fc0a        .inst   0xa691fc0a ; undefined
  1c:   7639e37d        .inst   0x7639e37d ; undefined
  20:   928b8571        mov     x17, #0xffffffffffffa3d4        // #-23596

Disassembly of section .note.Linux:

0000000000000000 <_note_9>:
   0:   00000006        udf     #6
   4:   00000004        udf     #4
   8:   00000101        udf     #257
   c:   756e694c        .inst   0x756e694c ; undefined
  10:   00000078        udf     #120
  14:   00000000        udf     #0

0000000000000018 <_note_8>:
  18:   00000006        udf     #6
  1c:   00000001        udf     #1
  20:   00000100        udf     #256
  24:   756e694c        .inst   0x756e694c ; undefined
  28:   00000078        udf     #120
  2c:   00000000        udf     #0

Disassembly of section .data:

0000000000000000 <faulty_fops>:
        ...

Disassembly of section .gnu.linkonce.this_module:

0000000000000000 <__this_module>:
        ...
  18:   6c756166        ldnp    d6, d24, Äx11, #-176Å
  1c:   00007974        udf     #31092
        ...

Disassembly of section .plt:

0000000000000000 <.plt>:
        ...

Disassembly of section .init.plt:

0000000000000000 <.init.plt>:
        ...

Disassembly of section .text.ftrace_trampoline:

0000000000000000 <.text.ftrace_trampoline>:
        ...

Disassembly of section .bss:

0000000000000000 <faulty_major>:
   0:   00000000        udf     #0

Disassembly of section .comment:

0000000000000000 <.comment>:
   0:   43434700        .inst   0x43434700 ; undefined
   4:   4228203a        .inst   0x4228203a ; undefined
   8:   646c6975        .inst   0x646c6975 ; undefined
   c:   746f6f72        .inst   0x746f6f72 ; undefined
  10:   32303220        orr     w0, w17, #0x1fff0000
  14:   31312e32        adds    w18, w17, #0xc4b
  18:   3933312d        strb    w13, Äx9, #3276Å
  1c:   38672d32        .inst   0x38672d32 ; undefined
  20:   34313365        cbz     w5, 6268c <cleanup_module+0x6250c>
  24:   62306231        .inst   0x62306231 ; undefined
  28:   31202938        adds    w24, w9, #0x80a
  2c:   2e332e31        uqsub   v17.8b, v17.8b, v19.8b
  30:   47000030        .inst   0x47000030 ; undefined
  34:   203a4343        .inst   0x203a4343 ; undefined
  38:   69754228        ldpsw   x8, x16, Äx17, #-88Å
  3c:   6f72646c        sqshlu  v12.2d, v3.2d, #50
  40:   3220746f        orr     w15, w3, #0x3fffffff
  44:   2e323230        usubw   v16.8h, v17.8h, v18.8b
  48:   312d3131        adds    w17, w9, #0xb4c
  4c:   2d323933        stp     s19, s14, Äx9, #-112Å
  50:   33653867        .inst   0x33653867 ; undefined
  54:   62313431        .inst   0x62313431 ; undefined
  58:   29386230        stp     w16, w24, Äx17, #-64Å
  5c:   2e313120        usubw   v0.8h, v9.8h, v17.8b
  60:   00302e33        .inst   0x00302e33 ; NYI


 # Analysis of objdump
 Disassembly of section .text:

0000000000000000 <faulty_write>:
   0:   d503245f        bti     c
   4:   d2800001        mov     x1, #0x0                        // #0
   8:   d2800000        mov     x0, #0x0                        // #0
   c:   d503233f        paciasp
  10:   d50323bf        autiasp
  14:   b900003f        str     wzr, Äx1Å
  18:   d65f03c0        ret
  1c:   d503201f        nop

Here, ar line 4: we are loading 0x0 into x1 and then at line 14: we are trying ti read from the pointed by x1
Which means, we are dereference a NULL pointer.

