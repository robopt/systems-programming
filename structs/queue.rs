use core::iter::Iterator;
use core::mem::replace;
use core::ops::FnMut;
use core::option::Option;
use core::ptr::{copy_nonoverlapping, null, write};
use core::slice::{from_raw_parts, from_raw_parts_mut, SliceExt};
use super::generic::dev::*;

// NOTE: can't think of a better way to do this to get the macro
#[path = "generic/dev.rs"] #[macro_use]
mod dev;


/// A fixed-size, FIFO/LILO array (queue).
pub struct Queue<'a, T: 'a> {
	data: &'a mut [T],
	first: usize,
	next: usize,
	full: bool,
}

struct QueueIter<'a, T: 'a>(&'a [T], &'a [T]);
impl<'a, T: 'a> Iterator for QueueIter<'a, T> {
	type Item = &'a T;

	#[inline]
	fn next(&mut self) -> Option<&'a T> {
		match self.0.len() {
			0 => Option::None,
			1 => unsafe {
				let ret = Option::Some(self.0.get_unchecked(0));
				self.0 = replace(&mut self.1, from_raw_parts(null(), 0));
				ret
			},
			_ => unsafe {
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

unsafe impl<'a, T: 'a> ContainerImpl<'a, T> for Queue<'a, T> {
	#[inline(always)]
	unsafe fn impl_construct(data: &mut [T], len: usize) -> Queue<T> {
		let full = len == data.len();
		Queue { data: data, first: 0, next: if full { 0 } else { len }, full: full }
	}

	#[inline(always)]
	fn impl_len(&self) -> usize {
		if self.full {
			self.data.len()
		} else if self.first <= self.next {
			self.next - self.first
		} else {
			self.data.len() - (self.first - self.next)
		}
	}

	#[inline(always)]
	fn impl_capacity(&self) -> usize {
		self.data.len()
	}

	#[inline(always)]
	fn impl_is_empty(&self) -> bool {
		self.first == self.next && !self.full
	}

	#[inline(always)]
	fn impl_is_full(&self) -> bool {
		self.full
	}

	#[inline(always)]
	unsafe fn impl_add(&mut self, value: T) {
		write(self.data.get_unchecked_mut(self.next), value);
		self.next += 1;
		if self.next == self.data.len() { self.next = 0 }
		if self.next == self.first { self.full = true }
	}

	#[inline(always)]
	unsafe fn impl_remove(&mut self) -> &mut T {
		self.full = false;
		let len = self.data.len();
		let ret = self.data.get_unchecked_mut(self.first);
		self.first += 1;
		if self.first == len { self.first = 0 }
		ret
	}

	#[inline(always)]
	unsafe fn impl_peek(&mut self) -> &mut T {
		self.data.get_unchecked_mut(self.first)
	}

	#[inline(always)]
	unsafe fn impl_clear<F>(&mut self, f: F) where F: FnMut(&mut [T]) {
		let mut f = f;
		if self.first <= self.next && !self.full {
			f(from_raw_parts_mut(self.data.get_unchecked_mut(self.first), self.next - self.first));
		} else {
			f(from_raw_parts_mut(self.data.get_unchecked_mut(self.first), self.data.len() - self.first));
			f(from_raw_parts_mut(self.data.get_unchecked_mut(0), self.next));
		}
		self.first = 0;
		self.next = 0;
	}

	#[inline(always)]
	unsafe fn impl_copy<F>(&mut self, data: &'a mut [T], f: F) where F: FnMut(&mut [T]) {
		if self.first <= self.next && !self.full {
			copy_nonoverlapping(self.data.get_unchecked_mut(self.first), data.get_unchecked_mut(0), self.next - self.first);
		} else {
			let len = self.data.len() - self.first;
			copy_nonoverlapping(self.data.get_unchecked_mut(self.first), data.get_unchecked_mut(0), len);
			copy_nonoverlapping(self.data.get_unchecked_mut(0), data.get_unchecked_mut(len), self.next);
		}
		let old = replace(&mut self.data, data);
		let mut f = f;
		f(old)
	}
}

impl<'a, T: 'a> Queue<'a, T> {
	#[inline(always)]
	fn iter(&self) -> QueueIter<T> {
		unsafe {
			if self.first <= self.next && !self.full {
				QueueIter(
					from_raw_parts(self.data.get_unchecked(self.first), self.next - self.first),
					from_raw_parts(null(), 0),
				)
			} else {
				QueueIter(
					from_raw_parts(self.data.get_unchecked(self.first), self.data.len() - self.first),
					from_raw_parts(self.data.get_unchecked(0), self.next),
				)
			}
		}
	}
}

container_debug!(Queue; );
