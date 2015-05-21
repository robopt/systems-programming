pub use core::cmp::min;
use core::fmt;
use core::marker::{Sized, Sync};
pub use core::mem::{size_of, transmute};
use core::mem::{zeroed, POST_DROP_USIZE};
use core::ops::{Deref, DerefMut, Drop};
pub use core::option::Option;
use core::ptr::{read, write, write_bytes};
use core::slice::SliceExt;
use super::container::{Container, Queue};


/// Implementation required for safe initialization of types.
pub trait Init {
	fn init(&mut self);
}

/// Implementation required for all allocatable types.
pub trait Alloc where Self: 'static + Sized + Sync {
	/// Creates an allocator for a type.
	fn allocator() -> &'static mut Allocator<Self>;
}

/// Easy way to implement Alloc.
///
/// # Example
///
/// ```
/// struct Thing(u64, u64, u64, u64);
/// impl_alloc!(Thing, 4000, 1000, "Alloc Failed", "Dealloc Failed");
/// ```
#[macro_export]
macro_rules! impl_alloc {
	($t:ident,
			$pool_size:expr, $refs_size:expr,
			$alloc_err:expr, $dealloc_err:expr) => {
		impl Alloc for $t {
			#[inline(always)]
			fn allocator() -> &'static mut Allocator<Self> {
				static mut pool: [usize; $pool_size] = [0; $pool_size];
				static mut refs: [usize; $refs_size] = [0; $refs_size];
				static mut alloc: Option<Allocator<$t>> = Option::None;
				unsafe {
					match alloc {
						Option::Some(ref mut a) => a,
						Option::None => {
							alloc = Option::Some(Allocator::new(
								transmute::<&mut usize, *mut $t>(&mut pool[0]),
								&mut refs[0],
								min(
									$pool_size * size_of::<usize>() / size_of::<$t>(),
									$refs_size
								),
								$alloc_err,
								$dealloc_err,
							));
							match alloc {
								Option::Some(ref mut a) => a,
								Option::None => unreachable!(),
							}
						}
					}
				}
			}
		}
	}
}

/// Simple mechanism for allocating objects from a pool.
pub struct Allocator<T: 'static + Alloc + Sync + Sized> {
	alloc_err: &'static str,
	dealloc_err: &'static str,
	refs: Queue<'static, *mut T>,
	pool: *mut T,
}

/// Marker for allocated types. Uses `unsafe_no_drop_flag` to ensure that it's
/// the same size as a pointer.
#[unsafe_no_drop_flag]
pub struct Allocated<T: 'static + Alloc + Sized> {
	data: &'static mut T
}
impl<T: 'static + Alloc + Sized> Deref for Allocated<T> {
	type Target = T;
	fn deref<'a>(&'a self) -> &'a T { self.data }
}
impl<T: 'static + Alloc + Sized> DerefMut for Allocated<T> {
	fn deref_mut<'a>(&'a mut self) -> &'a mut T { self.data }
}
impl<T: 'static + Alloc + Sized> Drop for Allocated<T> {
	fn drop(&mut self) {
		unsafe {
			let ptr = self.data as *const T;
			if ptr != (0 as *const T) && ptr as usize != POST_DROP_USIZE {
				read(&*self.data);
				<T as Alloc>::allocator().dealloc(self.data);
				self.data = zeroed();
			}
		}
	}
}

impl<T: 'static + Sized + Sync + Alloc + fmt::Debug> fmt::Debug for Allocated<T> {
	fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
		self.data.fmt(fmt)
	}
}

impl<T: Alloc> Allocator<T> {
	/// Constructs an allocator from a pool of objects, a place to store pointers
	/// to objects in the pool, and panic errors for allocate and deallocate.
	///
	/// # Safety
	///
	/// This is unsafe because it assumes that pool and refs are at least num in
	/// length and both zero.
	#[inline]
	pub unsafe fn new(
			pool: *mut T, refs: *mut usize, num: usize,
			alloc_err: &'static str, dealloc_err: &'static str) -> Allocator<T> {
		let refs = transmute::<*mut usize, *mut *mut T>(refs);
		for i in (0..num) {
			write(refs.offset(i as isize), pool.offset(i as isize))
		}
		Allocator {
			pool: pool,
			refs: Queue::from_existing_ptr(refs, num, num),
			alloc_err: alloc_err, dealloc_err: dealloc_err
		}
	}

	/// Returns the number of available objects in this allocator.
	#[inline(always)]
	pub fn available(&self) -> usize {
		self.refs.len()
	}

	/// Returns the same value as `available() > 0`, but may be faster if the
	/// implementation allows it.
	#[inline(always)]
	pub fn has_available(&self) -> bool {
		!self.refs.is_empty()
	}

	/// Allocates an object from its allocator.
	///
	/// # Panics
	///
	/// Panics if `!has_available()`.
	///
	/// # Safety
	///
	/// This is unsafe because it doesn't initialize the allocated object.
	#[inline(always)]
	pub unsafe fn alloc_uninitialized(&mut self) -> Allocated<T> {
		let value = self.refs.remove_or_panic(self.alloc_err);
		Allocated { data: transmute::<*mut T, &mut T>(value) }
	}

	/// Opitonally allocates an object from its allocator.
	///
	/// # Safety
	///
	/// This is unsafe because it doesn't initialize the allocated object.
	#[inline(always)]
	pub unsafe fn alloc_unitialized_option(&mut self) -> Option<Allocated<T>> {
		self.refs.remove().map(|value| {
			Allocated { data: transmute::<*mut T, &mut T>(value) }
		})
	}

	/// Allocates an object from its allocator.
	///
	/// # Panics
	///
	/// Panics if `!has_available()`.
	#[inline(always)]
	pub fn alloc(&mut self) -> Allocated<T> where T: Init {
		let value = self.refs.remove_or_panic(self.alloc_err);
		unsafe { (*value).init() };
		Allocated { data: unsafe { transmute::<*mut T, &mut T>(value) } }
	}

	/// Optionally allocates an object from its allocator.
	#[inline(always)]
	pub fn alloc_option(&mut self) -> Option<Allocated<T>> where T: Init {
		self.refs.remove().map(|value| {
			unsafe { (*value).init(); }
			Allocated { data: unsafe { transmute::<*mut T, &mut T>(value) } }
		})
	}

	#[inline(always)]
	unsafe fn dealloc(&mut self, value: *mut T) {
		write_bytes(value, 0, 1);
		self.refs.add_or_panic(value, self.dealloc_err)
	}
}

impl<T: 'static + Alloc + Sized> fmt::Debug for Allocator<T> where T: fmt::Debug {
	fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
		let pool_end = unsafe { self.pool.offset(self.refs.capacity() as isize) };
		fmt.debug_struct("Allocator")
			.field("available()", &self.available())
			.field("pool", &(self.pool..pool_end))
			.field("refs", &self.refs)
			.finish()
	}
}
