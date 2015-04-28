use super::allocator::*;
use super::common::*;

/// Number of registers which can be saved on the stack.
pub const STACK_SIZE: usize = 1024;
/// 32-bit stack.
pub struct Stack32 { pub data: [u32; STACK_SIZE] }
/// 64-bit stack.
pub struct Stack64 { pub data: [u64; STACK_SIZE] }

impl_alloc!(
	Stack32,
	MAX_PROCS, MAX_PROCS,
	"unable to allocate 32-bit stack",
	"unable to deallocate 32-bit stack"
);
impl_alloc!(
	Stack64,
	MAX_PROCS, MAX_PROCS,
	"unable to allocate 64-bit stack",
	"unable to deallocate 64-bit stack"
);
