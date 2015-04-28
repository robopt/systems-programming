use core::convert::Into;
use core::fmt;
use core::mem::transmute;
use super::container::slice::*;

const GDT32_CODE: u32 = 0x0010;
const GDT32_DATA: u32 = 0x0018;
const GDT32_STACK: u32 = 0x0020;

// TODO: FIX
const GDT64_CODE: u64 = 0x0010;
const GDT64_DATA: u64 = 0x0018;
const GDT64_STACK: u64 = 0x0020;

/// FLAGS register constants.
pub mod flags {
	/// Default value for eflags/rflags register
	pub const DEFAULT: u64 = (MUST_BE_1 | IF);

	/// Bits which are unused (all bits past bit 21)
	pub const RESERVED: u64 = !0b1111111111111111111111;
	/// Bits which must be 0
	pub const MUST_BE_0: u64 = (0b1 << 5) | (0b1 << 15);
	/// Bits which must be 1
	pub const MUST_BE_1: u64 = (0b1 << 1);

	/// CPUID instruction enable flag
	pub const ID:  u64 = (0b1 << 21);
	/// Virtual interrupt pending flag
	pub const VIP: u64 = (0b1 << 20);
	/// Virtual interrupt flag
	pub const VIF: u64 = (0b1 << 19);
	/// Alignment check flag
	pub const AC: u64 = (0b1 << 18);
	/// Virtual 8086 mode flag
	pub const VM: u64 = (0b1 << 17);
	/// Resume flag
	pub const RF: u64 = (0b1 << 16);
	/// Nested task flag
	pub const NT: u64 = (0b1 << 14);
	/// I/O privilege level flag
	pub const IOPL: u64 = (0b11 << 12);
	/// Overflow flag
	pub const OF: u64 = (0b1 << 11);
	/// Direction flag
	pub const DF: u64 = (0b1 << 10);
	/// Interrupt enable flag
	pub const IF: u64 = (0b1 << 9);
	/// Trap flag
	pub const TF: u64 = (0b1 << 8);
	/// Sign flag
	pub const SF: u64 = (0b1 << 7);
	/// Zero flag
	pub const ZF: u64 = (0b1 << 6);
	/// Adjust flag
	pub const AF: u64 = (0b1 << 4);
	/// Parity flag
	pub const PF: u64 = (0b1 << 2);
	/// Carry flag
	pub const CF: u64 = (0b1 << 0);
}

#[repr(C)]
pub struct Context32 {
	pub ss: u32,
	pub gs: u32,
	pub fs: u32,
	pub es: u32,
	pub ds: u32,
	pub edi: u32,
	pub esi: u32,
	pub ebp: u32,
	pub esp: u32,
	pub ebx: u32,
	pub edx: u32,
	pub ecx: u32,
	pub eax: u32,
	pub vector: u32,
	pub code: u32,
	pub eip: u32,
	pub cs: u32,
	pub eflags: u32,
}

#[repr(C)]
pub struct Context64 {
	pub ss: u64,
	pub gs: u64,
	pub fs: u64,
	pub es: u64,
	pub ds: u64,
	pub r15: u64,
	pub r14: u64,
	pub r13: u64,
	pub r12: u64,
	pub r11: u64,
	pub r10: u64,
	pub r9: u64,
	pub r8: u64,
	pub rdi: u64,
	pub rsi: u64,
	pub rbp: u64,
	pub rsp: u64,
	pub rbx: u64,
	pub rdx: u64,
	pub rcx: u64,
	pub rax: u64,
	pub vector: u64,
	pub code: u64,
	pub rip: u64,
	pub cs: u64,
	pub rflags: u64,
}

impl Context64 {
	#[inline(always)]
	pub fn new() -> Context64 {
		Context64 {
			ss: GDT64_STACK,
			gs: GDT64_DATA,
			fs: GDT64_DATA,
			es: GDT64_DATA,
			ds: GDT64_DATA,
			r15: 0,
			r14: 0,
			r13: 0,
			r12: 0,
			r11: 0,
			r10: 0,
			r9: 0,
			r8: 0,
			rdi: 0,
			rsi: 0,
			rbp: 0,
			rsp: 0,
			rbx: 0,
			rdx: 0,
			rcx: 0,
			rax: 0,
			vector: 0,
			code: 0,
			rip: 0,
			cs: GDT64_CODE,
			rflags: flags::DEFAULT,
		}
	}
}

impl Context32 {
	#[inline(always)]
	pub fn new() -> Context32 {
		Context32 {
			ss: GDT32_STACK,
			gs: GDT32_DATA,
			fs: GDT32_DATA,
			es: GDT32_DATA,
			ds: GDT32_DATA,
			edi: 0,
			esi: 0,
			ebp: 0,
			esp: 0,
			ebx: 0,
			edx: 0,
			ecx: 0,
			eax: 0,
			vector: 0,
			code: 0,
			eip: 0,
			cs: GDT32_CODE,
			eflags: flags::DEFAULT as u32,
		}
	}
}

impl Into<Context32> for Context64 {
	#[inline(always)]
	fn into(self) -> Context32 {
		Context32 {
			ss: self.ss as u32,
			gs: self.gs as u32,
			fs: self.fs as u32,
			es: self.es as u32,
			ds: self.ds as u32,
			edi: self.rdi as u32,
			esi: self.rsi as u32,
			ebp: self.rbp as u32,
			esp: self.rsp as u32,
			ebx: self.rbx as u32,
			edx: self.rdx as u32,
			ecx: self.rcx as u32,
			eax: self.rax as u32,
			vector: self.vector as u32,
			code: self.code as u32,
			eip: self.rip as u32,
			cs: self.cs as u32,
			eflags: self.rflags as u32,
		}
	}
}

impl Into<[u32; 18]> for Context32 {
	#[inline(always)]
	fn into(self) -> [u32; 18] {
		let mut slice = unsafe { transmute::<_, [u32; 18]>(self) };
		slice.reverse();
		slice
	}
}

impl Into<[u64; 26]> for Context64 {
	#[inline(always)]
	fn into(self) -> [u64; 26] {
		let mut slice = unsafe { transmute::<_, [u64; 26]>(self) };
		slice.reverse();
		slice
	}
}

impl fmt::Debug for Context32 {
	fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
		fmt.write_fmt(format_args!("\
			\n\t ss {:08x}  gs {:08x}  fs {:08x}  es {:08x}\
			\n\t ds {:08x} edi {:08x} esi {:08x} ebp {:08x}\
			\n\tesp {:08x} ebx {:08x} ebx {:08x} ecx {:08x}\
			\n\teax {:08x} vec {:08x} cod {:08x} eip {:08x}\
			\n\t cs {:08x} efl {:08x}\
			\n\
		",
			self.ss,  self.gs,     self.fs,   self.es,
			self.ds,  self.edi,    self.esi,  self.ebp,
			self.esp, self.ebx,    self.edx,  self.ecx,
			self.eax, self.vector, self.code, self.eip,
			self.cs,  self.eflags
		))
	}
}
