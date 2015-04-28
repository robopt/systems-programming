use core::convert::Into;
use super::allocator::*;
use super::context::*;
use super::stack::*;
use super::super::syscall::default_exit;
use super::container::*;
use super::common::*;

/// 64-bit process control block.
#[repr(C)]
pub struct Process64 {
	context: &'static mut Context64,
	stack: Allocated<Stack64>,
	wakeup: u64,
	pid: i16,
	ppid: i16,
	prio: u8,
	state: u8,
	quantum: u8,
	default_quantum: u8,
}

impl Init for Process64 {
	fn init(&mut self) {
		unsafe {
			self.stack = Stack64::allocator().alloc_uninitialized();
			let mut stack = Vec::from_empty(&mut (*self.stack).data);
			stack.add_or_panic(0, "");
			stack.add_or_panic(transmute::<_, u64>(default_exit), "");
			stack.extend_or_panic(&mut Into::<[u64; 26]>::into(Context64::new()), "");
			self.context = transmute::<&mut u64, &mut Context64>(stack.peek_mut().unwrap());
		}
	}
}

impl_alloc!(
	Process64,
	MAX_PROCS, MAX_PROCS,
	"unable to allocate PCB",
	"unable to deallocate PCB"
);
