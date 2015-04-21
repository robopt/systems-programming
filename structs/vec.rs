use core::iter::Iterator;
use core::mem::replace;
use core::ops::FnMut;
use core::option::Option;
use core::ptr::{copy_nonoverlapping, write};
use core::slice::{from_raw_parts, from_raw_parts_mut, SliceExt};
use super::generic::dev::*;

// NOTE: can't think of a better way to do this to get the macro
#[path = "generic/dev.rs"] #[macro_use]
mod dev;


/// A fixed-size, FILO/LIFO array (stack).
pub struct Vec<'a, T: 'a> {
	data: &'a mut [T],
	len: usize,
}

struct VecIter<'a, T: 'a>(&'a [T]);
impl<'a, T: 'a> Iterator for VecIter<'a, T> {
	type Item = &'a T;

	#[inline]
	fn next(&mut self) -> Option<&'a T> {
		if self.0.len() == 0 {
			Option::None
		} else {
			unsafe {
				let ret = Option::Some(self.0.get_unchecked(0));
				self.0 = from_raw_parts(
					self.0.get_unchecked(1) as *const T,
					self.0.len() - 1
				);
				ret
			}
		}
	}
}

unsafe impl<'a, T: 'a> ContainerImpl<'a, T> for Vec<'a, T> {
	#[inline(always)]
	unsafe fn impl_construct(data: &mut [T], len: usize) -> Vec<T> {
		Vec { data: data, len: len }
	}

	#[inline(always)]
	fn impl_len(&self) -> usize {
		self.len
	}

	#[inline(always)]
	fn impl_capacity(&self) -> usize {
		self.data.len()
	}

	#[inline(always)]
	fn impl_is_empty(&self) -> bool {
		self.len == 0
	}

	#[inline(always)]
	fn impl_is_full(&self) -> bool {
		self.len == self.data.len()
	}

	#[inline(always)]
	unsafe fn impl_add(&mut self, value: T) {
		write(self.data.get_unchecked_mut(self.len), value);
		self.len += 1;
	}

	#[inline(always)]
	unsafe fn impl_remove(&mut self) -> &mut T {
		self.len -= 1;
		self.data.get_unchecked_mut(self.len)
	}

	#[inline(always)]
	unsafe fn impl_peek(&mut self) -> &mut T {
		self.data.get_unchecked_mut(self.len - 1)
	}

	#[inline(always)]
	unsafe fn impl_clear<F>(&mut self, f: F) where F: FnMut(&mut [T]) {
		let mut f = f;
		f(from_raw_parts_mut(self.data.get_unchecked_mut(0), self.len));
		self.len = 0
	}

	#[inline(always)]
	unsafe fn impl_copy<F>(&mut self, data: &'a mut [T], f: F) where F: FnMut(&mut [T]) {
		copy_nonoverlapping(self.data.get_unchecked_mut(0), data.get_unchecked_mut(0), self.len);
		let old = replace(&mut self.data, data);
		let mut f = f;
		f(from_raw_parts_mut(old.get_unchecked_mut(0), self.len));
	}
}

impl<'a, T: 'a> Vec<'a, T> {
	#[inline(always)]
	fn iter(&self) -> VecIter<T> {
		unsafe { VecIter(from_raw_parts(self.data.get_unchecked(0), self.len)) }
	}
}

container_debug!(Vec; );
