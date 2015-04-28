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
				self.0 = replace(&mut self.1, self.0.trunc_unchecked(0));
				ret
			},
			_ => unsafe {
				let ret = Option::Some(self.0.get_unchecked(0));
				self.0 = self.0.trunc_unchecked(self.0.len() - 1);
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

	#[inline]
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

	#[inline]
	unsafe fn impl_add(&mut self, value: T) {
		self.data.write_unchecked(self.next, value);
		self.next += 1;
		if self.next == self.data.len() { self.next = 0 }
		if self.next == self.first { self.full = true }
	}

	#[inline]
	fn impl_extend(&mut self, values: &mut [T]) -> usize {
		unsafe {
			// TODO: refactor
			// X _ X or _ _ X: free space located in middle of queue
			if self.next < self.first {
				let len = min(values.len(), self.first - self.next);
				self.data.copy_unchecked(self.next, values.trunc_unchecked(len));
				self.next += len;
				len

			// X X _ or _ _ _: enough free space located at right end of queue
			} else if values.len() < self.data.len() - self.next {
				self.data.copy_unchecked(self.next, values);
				self.next += values.len();
				values.len()

			// _ X _: space required at both ends of queue
			} else {
				let len1 = self.data.len() - self.next;
				self.data.copy_unchecked(self.next, values.trunc_unchecked(len1));
				let values = values.offset_unchecked(len1);
				let len2 = min(self.first, values.len());
				self.data.copy_unchecked(0, values.trunc_unchecked(len2));
				len1 + len2
			}
		}
	}

	#[inline]
	unsafe fn impl_remove(&mut self) -> &mut T {
		self.full = false;
		let len = self.data.len();
		let ret = self.data.get_unchecked_mut(self.first);
		self.first += 1;
		if self.first == len { self.first = 0 }
		ret
	}

	#[inline]
	unsafe fn impl_forget<F>(&mut self, n: usize, mut f: F) -> usize where F: FnMut(&mut T) {
		// contiguous data
		if self.first <= self.next && !self.full {
			let newnext = min(self.first, usize::saturating_sub(self.next, n));
			for i in (newnext..self.next) {
				f(self.data.get_unchecked_mut(i))
			}
			newnext - self.first

		// non-contiguous data
		} else {
			for i in (usize::saturating_sub(self.next, n)..self.next) {
				f(self.data.get_unchecked_mut(i))
			}
			if n > self.next {
				let oldnext = self.next;
				self.next = min(self.first, self.data.len() - self.next - n);
				for i in self.next..self.data.len() {
					f(self.data.get_unchecked_mut(i))
				}
				oldnext + self.data.len() - self.next
			} else {
				self.next
			}
		}
	}

	#[inline(always)]
	unsafe fn impl_peek(&mut self) -> &mut T {
		self.data.get_unchecked_mut(self.first)
	}

	#[inline]
	unsafe fn impl_clear<F>(&mut self, mut f: F) where F: FnMut(&mut [T]) {
		if self.first <= self.next && !self.full {
			f(self.data.slice_unchecked_mut(self.first, self.next));
		} else {
			f(self.data.offset_unchecked_mut(self.first));
			f(self.data.trunc_unchecked_mut(self.next));
		}
		self.first = 0;
		self.next = 0;
	}

	#[inline]
	unsafe fn impl_copy<F>(&mut self, data: &'a mut [T], mut f: F) where F: FnMut(&mut [T]) {
		if self.first <= self.next && !self.full {
			data.copy_unchecked(0, self.data.slice_unchecked(self.first, self.next));
		} else {
			let slice1 = self.data.offset_unchecked(self.next);
			let slice2 = self.data.trunc_unchecked(self.first);
			data.copy_unchecked(0, slice1);
			data.copy_unchecked(slice1.len(), slice2);
		}
		let old = replace(&mut self.data, data);
		f(old)
	}
}

impl<'a, T: 'a> Queue<'a, T> {
	#[inline(always)]
	fn iter(&self) -> QueueIter<T> {
		unsafe {
			if self.first <= self.next && !self.full {
				QueueIter(
					self.data.slice_unchecked(self.first, self.next),
					self.data.trunc_unchecked(0)
				)
			} else {
				QueueIter(
					self.data.offset_unchecked(self.first),
					self.data.trunc_unchecked(self.next)
				)
			}
		}
	}
}

container_debug!(Queue; );
