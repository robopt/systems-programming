pub use core::fmt;
use core::marker::Sized;
use core::ops::FnMut;

/// Implementation required to have a basic container.
pub unsafe trait ContainerImpl<'a, T: 'a> where Self: Sized {
	/// Constructs the container from an existing array of data. Should not
	/// check if `len <= data.len()`, should not drop the contents of
	/// `data[len..]`, and should preferably not copy the data or allocate/use
	/// more memory than what is provided.
	unsafe fn impl_construct(data: &'a mut [T], len: usize) -> Self;

	/// Returns the current number of elements in the container in O(1) time.
	fn impl_len(&self) -> usize;

	/// Returns the maximum number of elements that the container can hold in O(1)
	/// time.
	fn impl_capacity(&self) -> usize;

	/// Returns the same value as `len() > 0`, but should be faster if the
	/// implementation allows it.
	fn impl_is_empty(&self) -> bool;

	/// Returns the same value as `len() == capacity()`, but should be faster
	/// if the implementation allows it.
	fn impl_is_full(&self) -> bool;

	/// Returns a mutable reference to the next element in the container which
	/// would be removed by a call to `remove()`. Should not check if the
	/// container is empty.
	unsafe fn impl_peek(&mut self) -> &mut T;

	/// Marks an element as removed from the container and returns a pointer to
	/// its current location in memory. Should not check if the container is empty
	/// or modify the value at the pointer.
	unsafe fn impl_remove(&mut self) -> &mut T;

	/// Adds an element to the container. Should not check if the container is
	/// full.
	unsafe fn impl_add(&mut self, value: T);

	/// Marks a container as empty after running a function on all of its
	/// elements. Should not modify the container's data besides passing them into
	/// the function.
	unsafe fn impl_clear<F>(&mut self, f: F) where F: FnMut(&mut [T]);

	/// Copies the container's data to a new buffer and runs a given function on
	/// the original buffer. Should not check if `self.len() <= data.len()`
	/// and should not modify the data besides passing them into the function.
	/// Should preferably make the data contiguous after the copy, if possible.
	/// The buffers will not overlap.
	unsafe fn impl_copy<F>(&mut self, data: &'a mut [T], f: F) where F: FnMut(&mut [T]);
}

/// Implements Debug for a container, displaying the contents of the container
/// in a DebugList.
///
/// # Examples
///
/// ```
/// container_debug(Queue; )
/// container_debug(PrioQueue; Priority)
/// container_debug(PrioQueue2; Priority, Priority2)
/// ````
#[macro_export]
macro_rules! container_debug {
	($name:ident; $($constraint:ident),*) => {
		impl<'a, T> fmt::Debug for $name<'a, T> where T: fmt::Debug $(+ $constraint)* {
			fn fmt(&self, fmt: &mut fmt::Formatter) -> fmt::Result {
				self.iter().fold(fmt.debug_list(), |b, e| b.entry(e)).finish()
			}
		}
	}
}
