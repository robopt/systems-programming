pub mod dev;
use core::mem::transmute;
use core::ptr::{read, read_and_zero, write_bytes};
use core::slice::{from_raw_parts_mut, SliceExt};
use core::option::Option;
use self::dev::ContainerImpl;

/// Generic container; automatically extends ContainerImpl.
pub trait Container<'a, T: 'a> where Self: ContainerImpl<'a, T> {
	/// Constructs a container an existing array of data.
	#[inline(always)]
	fn from_full(data: &'a mut [T]) -> Self {
		let len = data.len();
		unsafe { <Self as ContainerImpl<T>>::impl_construct(data, len) }
	}

	/// Constructs a container from an existing array of data.
	///
	/// # Panics
	///
	/// Panics if `len > data.len()`.
	///
	/// # Safety
	///
	/// This is unsafe because the elements of `data[len..]` are not dropped, even
	/// if `len < data.len()`.
	#[inline(always)]
	unsafe fn from_existing(data: &'a mut [T], len: usize) -> Self {
		debug_assert!(len <= data.len());
		<Self as ContainerImpl<T>>::impl_construct(data, len)
	}

	/// Constructs a container from an empty array of data.
	///
	/// # Safety
	///
	/// This is unsafe because the elements of `data` are not dropped.
	#[inline(always)]
	unsafe fn from_empty(data: &'a mut [T]) -> Self {
		<Self as ContainerImpl<T>>::impl_construct(data, 0)
	}

	/// Constructs a container from a pointer to an existing array of data.
	///
	/// # Safety
	///
	/// This is unsafe because it assumes that `data` is `cap` long and
	/// initialized.
	#[inline(always)]
	unsafe fn from_full_ptr(data: *mut T, cap: usize) -> Self {
		<Self as ContainerImpl<T>>::impl_construct(from_raw_parts_mut(data, cap), cap)
	}

	/// Constructs a container from a pointer to an existing array of data.
	///
	/// # Safety
	///
	/// This is unsafe because it assumes that the array is uninitialised after
	/// the first `len` elements and can contain at least `cap` elements.
	#[inline(always)]
	unsafe fn from_existing_ptr(data: *mut T, cap: usize, len: usize) -> Self {
		<Self as ContainerImpl<T>>::impl_construct(from_raw_parts_mut(data, cap), len)
	}

	/// Constructs a container from a pointer to an uninitialised array.
	///
	/// # Safety
	///
	/// This is unsafe because it assumes that the array is uninitialised and can
	/// contain at least `cap` elements.
	#[inline(always)]
	unsafe fn from_empty_ptr(data: *mut T, cap: usize) -> Self {
		<Self as ContainerImpl<T>>::impl_construct(from_raw_parts_mut(data, cap), 0)
	}

	/// Returns the current number of elements in the container in O(1) time.
	#[inline(always)]
	fn len(&self) -> usize { self.impl_len() }

	/// Returns the maximum number of elements that the container can hold in O(1)
	/// time.
	#[inline(always)]
	fn capacity(&self) -> usize { self.impl_capacity() }

	/// Returns the same value as `len() > 0`, but may be faster if the
	/// implementation allows it.
	#[inline(always)]
	fn is_empty(&self) -> bool { self.impl_is_empty() }

	/// Returns the same value as `len() == capacity()`, but may be faster
	/// if the implementation allows it.
	#[inline(always)]
	fn is_full(&self) -> bool { self.impl_is_full() }

	/// Returns a mutable reference to the next element in the container which
	/// would be removed by a call to `remove()`.
	#[inline(always)]
	fn peek_mut(&mut self) -> Option<&mut T> {
		if self.is_empty() {
			Option::None
		} else {
			unsafe { Option::Some(self.impl_peek()) }
		}
	}

	/// Returns a mutable reference to the next element in the container which
	/// would be removed by a call to `remove()`.
	///
	/// # Panics
	///
	/// Panics with `err` if `is_empty()`.
	#[inline(always)]
	fn peek_mut_or_panic(&mut self, err: &'static str) -> &mut T {
		debug_assert!(!self.is_empty(), "{}", err);
		unsafe { self.impl_peek() }
	}

	/// Returns an immutable reference to the next element in the container which
	/// would be removed by a call to `remove()`.
	#[inline(always)]
	fn peek(&self) -> Option<&T> {
		if self.is_empty() {
			Option::None
		} else {
			unsafe { Option::Some((transmute::<&Self, &mut Self>(self)).impl_peek()) }
		}
	}
	/// Returns an immutable reference to the next element in the container which
	/// would be removed by a call to `remove()`.
	///
	/// # Panics
	///
	/// Panics with `err` if `is_empty()`.
	#[inline(always)]
	fn peek_or_panic(&self, err: &'static str) -> &T {
		debug_assert!(!self.is_empty(), "{}", err);
		unsafe { (transmute::<&Self, &mut Self>(self)).impl_peek() }
	}

	/// Removes a value from the container and returns the value, or `None` if the
	/// container is empty.
	#[inline(always)]
	fn remove(&mut self) -> Option<T> {
		if self.is_empty() {
			Option::None
		} else {
			unsafe { Option::Some(read(self.impl_remove())) }
		}
	}

	/// Removes a value from the container and returns the value.
	///
	/// # Panics
	///
	/// Panics with `err` if `is_empty()`.
	#[inline(always)]
	fn remove_or_panic(&mut self, err: &'static str) -> T {
		debug_assert!(!self.is_empty(), "{}", err);
		unsafe { read(self.impl_remove()) }
	}

	/// Removes a value from the container and returns the value, or `None` if the
	/// container is empty. Zeroes out the value's original location in memory.
	#[inline(always)]
	fn remove_and_zero(&mut self) -> Option<T> {
		if self.is_empty() {
			Option::None
		} else {
			unsafe { Option::Some(read(self.impl_remove())) }
		}
	}

	/// Removes a value from the container and returns the value. Zeroes out the
	/// value's original location in memory.
	///
	/// # Panics
	///
	/// Panics with `err` if `is_empty()`.
	#[inline(always)]
	fn remove_and_zero_or_panic(&mut self, err: &'static str) -> T {
		debug_assert!(!self.is_empty(), "{}", err);
		unsafe { read_and_zero(self.impl_remove()) }
	}

	/// Removes multiple values from the container and returns how many values
	/// were removed.
	#[inline(always)]
	fn forget(&mut self, n: usize) -> usize {
		unsafe { self.impl_forget(n, |x| { read(x); }) }
	}

	/// Removes multiple values from the container and returns how many values
	/// were removed. Zeroes out the values' original locations in memory.
	#[inline(always)]
	fn forget_and_zero(&mut self, n: usize) -> usize {
		unsafe { self.impl_forget(n, |x| { read_and_zero(x); }) }
	}

	/// Adds an element to the container.
	///
	/// # Panics
	///
	/// Panics with `err` if `is_full()`.
	#[inline(always)]
	fn add_or_panic(&mut self, value: T, err: &'static str) {
		debug_assert!(!self.is_full(), "{}", err);
		unsafe { self.impl_add(value) }
	}

	/// Adds multiple elements to the container and returns how many values were
	/// added. May be faster than multiple adds if the implementation allows it.
	///
	/// # Safety
	///
	/// This is unsafe because it copies the original values in the array instead
	/// of moving them.
	#[inline(always)]
	fn extend(&mut self, values: &mut [T]) -> usize {
		self.impl_extend(values)
	}

	/// Adds multiple elements to the container. May be faster than multiple adds
	/// if the implementation allows it.
	///
	/// # Panics
	///
	/// Panics if the value was partially added to the container.
	///
	/// # Safety
	///
	/// This is unsafe because it copies the original values in the array instead
	/// of moving them.
	#[inline(always)]
	fn extend_or_panic(&mut self, values: &mut [T], err: &'static str) {
		let n = self.impl_extend(values);
		debug_assert!(n == values.len(), "{}", err);
	}

	/// Drops all of the elements in the container.
	#[inline(always)]
	fn clear(&mut self) {
		unsafe { self.impl_clear(|slice| for x in slice { read(x); }) }
	}

	/// Drops all of the elements in the container. Zeroes out the container's
	/// buffer.
	#[inline(always)]
	fn clear_and_zero(&mut self) {
		unsafe { self.impl_clear(|slice| for x in slice { read_and_zero(x); }) }
	}

	/// Copies the container's data to a new pointer. Guarantees that the data
	/// will be contiguous after the copy.
	///
	/// # Panics
	///
	/// Panics if `data.len() < self.len()`.
	///
	/// # Safety
	///
	/// This is unsafe because there will be two mutable references to the same
	/// data, and because `data[..self.len]` is not dropped.
	#[inline(always)]
	unsafe fn move_data(&mut self, data: &'a mut [T]) {
		debug_assert!(self.len() <= data.len());
		self.impl_copy(data, |_| ());
	}

	/// Copies the container's data to a new pointer. Guarantees that the data
	/// will be contiguous after the copy.
	///
	/// # Panics
	///
	/// Panics if `data.len() < self.len()`.
	///
	/// # Safety
	///
	/// This is unsafe because `data[..self.len]` is not dropped.
	#[inline(always)]
	unsafe fn move_data_and_zero(&mut self, data: &'a mut [T]) {
		debug_assert!(data.len() < self.len());
		self.impl_copy(data, |x| {
			let len = x.len();
			write_bytes(x.get_unchecked_mut(0) as *mut T, 0, len)
		});
	}
}

impl<'a, T: 'a, A: ContainerImpl<'a, T>> Container<'a, T> for A {}
