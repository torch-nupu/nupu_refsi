// RUN: clc --device "RefSi M1" %s

__kernel void fn() {

  riscv_nu_nop();

  __asm__ __volatile__("nu.nop");
};
