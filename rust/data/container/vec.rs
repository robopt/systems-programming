use core::cmp::min;
use core::iter::Iterator;
use core::mem::replace;
use core::ops::FnMut;
use core::option::Option;
use super::generic::dev::*;
use super::slice::*;

// NOTE: can't think of a better way to do this to get the macro
#[path = "generic/dev.rs"] #[macro_use]
mod dev;


/// A fixed-size, FILO/LIFO array (stack). Uses x86 convention of inserting to
/// the positive side of the array first.
pub struct Vec<'a, T: 'a> {
	data: &'a mut [T],
	last: usize,
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
				let len = self.0.len() - 1;
				let ret = Option::Some(self.0.get_unchecked(len));
				self.0 = self.0.trunc_unchecked(len);
				ret
			}
		}
	}
}

unsafe impl<'a, T: 'a> ContainerImpl<'a, T> for Vec<'a, T> {
	#[inline]
	unsafe fn impl_construct(data: &mut [T], len: usize) -> Vec<T> {
		let cap = data.len();
		Vec { data: data, last: cap - len }
	}

	#[inline]
	fn impl_len(&self) -> usize {
		self.data.len() - self.last
	}

	#[inline]
	fn impl_capacity(&self) -> usize {
		self.data.len()
	}

	#[inline]
	fn impl_is_empty(&self) -> bool {
		self.last == self.data.len()
	}

	#[inline]
	fn impl_is_full(&self) -> bool {
		self.last == 0
	}

	#[inline]
	unsafe fn impl_add(&mut self, value: T) {
		self.last -= 1;
		self.data.write_unchecked(self.last, value);
	}

	#[inline]
	fn impl_extend(&mut self, values: &mut [T]) -> usize {
		let len = min(self.last, values.len());
		self.last -= len;
		unsafe {
			self.data.copy_unchecked(self.last, values.trunc_unchecked(len))
		}
		len
	}

	#[inline]
	unsafe fn impl_remove(&mut self) -> &mut T {
		let ret = self.data.get_unchecked_mut(self.last);
		self.last += 1;
		ret
	}

	#[inline]
	unsafe fn impl_forget<F>(&mut self, n: usize, mut f: F) -> usize where F: FnMut(&mut T) {
		let stop = if self.data.len() - self.last > n {
			self.data.len()
		} else {
			self.last + n
		};
		for i in (self.last..stop) {
			f(self.data.get_unchecked_mut(i))
		}
		stop - self.last
	}

	#[inline]
	unsafe fn impl_peek(&mut self) -> &mut T {
		self.data.get_unchecked_mut(self.last)
	}

	#[inline]
	unsafe fn impl_clear<F>(&mut self, mut f: F) where F: FnMut(&mut [T]) {
		f(self.data.offset_unchecked_mut(self.last));
		self.last = self.data.len()
	}

	#[inline]
	unsafe fn impl_copy<F>(&mut self, data: &'a mut [T], mut f: F) where F: FnMut(&mut [T]) {
		let last = self.last + data.len() - self.data.len();
		data.copy_unchecked(last, self.data.offset_unchecked(self.last));
		let old = replace(&mut self.data, data);
		f(old.offset_unchecked_mut(self.last));
	}
}

impl<'a, T: 'a> Vec<'a, T> {
	#[inline]
	fn iter(&self) -> VecIter<T> {
		unsafe { VecIter(self.data.offset_unchecked(self.last)) }
	}
}

container_debug!(Vec; );
