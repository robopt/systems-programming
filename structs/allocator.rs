pub use core::cmp::min;
use core::default::Default;
use core::fmt;
use core::marker::Sized;
use core::mem::{transmute, zeroed, POST_DROP_USIZE};
use core::ops::{Deref, DerefMut, Drop};
use core::ptr::{read, write};
use core::slice::SliceExt;
use super::generic::Container;
use super::queue::Queue;


/// Implementation required for all allocatable types.
pub trait Alloc where Self: 'static + Sized {
	/// Initializes allocation for a type, making future calls to any `Alloc`
	/// functions safe. Should be `inline(always)`.
	fn init_allocator();

	/// Gets the allocator for a type. Should be `inline(always)``.
	fn allocator() -> &'static mut Allocator<Self>;

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
	unsafe fn alloc_uninitialized() -> Allocated<Self> {
		<Self as Alloc>::allocator().alloc_uninitialized()
	}

	/// Allocates an object from its allocator.
	///
	/// # Panics
	///
	/// Panics if `!has_available()`.
	#[inline(always)]
	fn alloc() -> Allocated<Self> where Self: Default {
		<Self as Alloc>::allocator().alloc()
	}

	/// Returns the number of available objects in this allocator.
	#[inline(always)]
	fn available(&self) -> usize {
		<Self as Alloc>::allocator().available()
	}

	/// Returns the same value as `available() > 0`, but may be faster if the
	/// implementation allows it.
	#[inline(always)]
	fn has_available(&self) -> bool {
		<Self as Alloc>::allocator().has_available()
	}
}

/// Easy way to implement Alloc.
///
/// # Example
///
/// ```
/// struct Thing(u64, u64, u64, u64);
/// impl_alloc!(Thing, 4000, 1000, "Alloc Failed", "Dealloc Failed");
/// ````
#[macro_export]
macro_rules! impl_alloc {
	($t:ident,
			$pool_size:expr, $refs_size:expr,
			$alloc_err:expr, $dealloc_err:expr) => {
		static mut my_allocator: Option<Allocator<$t>> = None;
		impl Alloc for $t {

			#[inline(always)]
			fn init_allocator() {
				static mut pool: [usize; $pool_size] = [0; $pool_size];
				static mut refs: [usize; $refs_size] = [0; $refs_size];
				unsafe {
					my_allocator = Some(
						Allocator::new(
							transmute::<&mut usize, *mut $t>(pool.get_unchecked_mut(0)),
							refs.get_unchecked_mut(0),
							min($pool_size * size_of::<usize>() / size_of::<$t>(), $refs_size),
							$alloc_err,
							$dealloc_err,
						)
					)
				};
			}

			#[inline(always)]
			fn allocator() -> &'static mut Allocator<$t> {
				// because an Allocator contains slice types, it won't ever be zero if
				// it's initialized, therefore, the option type for the allocator will
				// be the same size as the contained type. so, we can just transmute to
				// get a valid pointer, and None will just give us an (invalid) null
				// pointer
				unsafe {
					debug_assert!(!my_allocator.is_none());
					transmute::<&mut Option<Allocator<$t>>, &mut Allocator<$t>>(
						&mut my_allocator
					)
				}
			}
		}
	}
}

/// Simple mechanism for allocating objects from a pool.
pub struct Allocator<T: 'static + Alloc + Sized> {
	alloc_err: &'static str,
	dealloc_err: &'static str,
	refs: Queue<'static, *mut T>,
	pool: *mut T,
}

// Technically untrue, but we don't have threads, so...
unsafe impl<T: 'static + Alloc + Sized> Sync for Allocator<T> {}

/// Marker for allocated types. Uses `unsafe_no_drop_flag` to ensure that it's
/// the same size as a pointer.
#[unsafe_no_drop_flag]
struct Allocated<T: 'static + Alloc + Sized> {
	data: &'static mut T
}
impl<T: 'static + Alloc + Sized> Deref for Allocated<T> {
	type Target = T;
	fn deref<'a>(&'a self) -> &'a T { self.data }
}
impl<T: 'static + Alloc + Sized> DerefMut for Allocated<T> {
	fn deref_mut<'a>(&'a mut self) -> &'a mut T { self.data }
}
#[unsafe_destructor]
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

impl<T: Alloc> Allocator<T> {
	/// Constructs an allocator from a pool of objects, a place to store pointers
	/// to objects in the pool, and panic errors for allocate and deallocate.
	///
	/// # Safety
	///
	/// This is unsafe because it assumes that pool and refs are at least num in
	/// length and both uninitialized.
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

	#[inline(always)]
	fn available(&self) -> usize {
		self.refs.len()
	}

	#[inline(always)]
	fn has_available(&self) -> bool {
		!self.refs.is_empty()
	}

	#[inline(always)]
	unsafe fn alloc_uninitialized(&mut self) -> Allocated<T> {
		let value = self.refs.remove_or_panic(self.alloc_err);
		Allocated { data: transmute::<*mut T, &mut T>(value) }
	}

	#[inline(always)]
	fn alloc(&mut self) -> Allocated<T> where T: Default {
		let value = self.refs.remove_or_panic(self.alloc_err);
		unsafe { write(value, Default::default()) };
		Allocated { data: unsafe { transmute::<*mut T, &mut T>(value) } }
	}

	#[inline(always)]
	unsafe fn dealloc(&mut self, value: *mut T) {
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
