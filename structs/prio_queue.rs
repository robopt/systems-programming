use core::cmp::{Ord, Ordering};
use core::iter::Iterator;
use core::mem::{replace, transmute};
use core::ops::FnMut;
use core::option::Option;
use core::ptr::{copy_nonoverlapping, write};
use core::slice::{from_raw_parts, from_raw_parts_mut, SliceExt};
use super::generic::dev::*;
use super::common::*;

// NOTE: can't think of a better way to do this to get the macro
#[path = "generic/dev.rs"] #[macro_use]
mod dev;


/// Trait for determining priority.
pub trait Priority where <Self as Priority>::Prio: Ord {
	type Prio;
	fn priority(&self) -> Self::Prio;
	fn cmp_prio(&self, other: &Self) -> Ordering { self.priority().cmp(&other.priority()) }
	fn lt_prio(&self, other: &Self) -> bool { self.priority() <  other.priority() }
	fn gt_prio(&self, other: &Self) -> bool { self.priority() >  other.priority() }
	fn le_prio(&self, other: &Self) -> bool { self.priority() <= other.priority() }
	fn ge_prio(&self, other: &Self) -> bool { self.priority() >= other.priority() }
	fn eq_prio(&self, other: &Self) -> bool { self.priority() == other.priority() }
	fn ne_prio(&self, other: &Self) -> bool { self.priority() != other.priority() }
}

/// Compares and returns the minimum of two values, by priority.
pub fn prio_min<T: Priority>(x: T, y: T) -> T { if x.le_prio(&y) { x } else { y } }

/// Compares and returns the maximum of two values, by priority.
pub fn prio_max<T: Priority>(x: T, y: T) -> T { if x.ge_prio(&y) { x } else { y } }


// Implement priority for all integer types. Floating-point doesn't have a
// default implementation because order may not be defined.
macro_rules! default_prio {
	($t:ty) => {
		impl Priority for $t {
			type Prio = $t;
			fn priority(&self) -> $t { *self }
		}
	}
}
default_prio!(i8);
default_prio!(i16);
default_prio!(i32);
default_prio!(i64);
default_prio!(isize);
default_prio!(u8);
default_prio!(u16);
default_prio!(u32);
default_prio!(u64);
default_prio!(usize);


/// A fixed-size, greatest-priority-first queue.
pub struct PrioQueue<'a, T: 'a + Priority> {
	data: &'a mut [T],
	len: usize,
}

struct PrioQueueIter<'a, T: 'a>(&'a [T]);
impl<'a, T: 'a> Iterator for PrioQueueIter<'a, T> {
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

unsafe impl<'a, T: 'a + Priority> ContainerImpl<'a, T> for PrioQueue<'a, T> {
	#[inline(always)]
	unsafe fn impl_construct(data: &mut [T], len: usize) -> PrioQueue<T> {
		let mut q = PrioQueue { data: data, len: len };
		q.heapify();
		q
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
		let oldlen = self.len;
		self.len += 1;
		self.sift_up(oldlen);
	}

	#[inline(always)]
	unsafe fn impl_remove(&mut self) -> &mut T {
		self.len -= 1;
		let len = self.len;
		swap_unchecked(self.data, 0, len);
		self.sift_down(0, len);
		self.data.get_unchecked_mut(self.len)
	}

	#[inline(always)]
	unsafe fn impl_peek(&mut self) -> &mut T {
		self.data.get_unchecked_mut(0)
	}

	#[inline(always)]
	unsafe fn impl_clear<F>(&mut self, mut f: F) where F: FnMut(&mut [T]) {
		f(from_raw_parts_mut(self.data.get_unchecked_mut(0), self.len));
		self.len = 0;
	}

	#[inline(always)]
	unsafe fn impl_copy<F>(&mut self, data: &'a mut [T], mut f: F) where F: FnMut(&mut [T]) {
		copy_nonoverlapping(self.data.get_unchecked_mut(0), data.get_unchecked_mut(0), self.len);
		let old = replace(&mut self.data, data);
		f(old)
	}
}

impl<'a, T: 'a + Priority> PrioQueue<'a, T> {
	#[inline]
	fn sift_down(&mut self, idx: usize, len: usize) {
		unsafe {
			let mut i = idx;
			let mut j = (i << 1) + 1;
			while j < len {
				let k = j + 1;
				if k < len && self.data.get_unchecked(j).le_prio(self.data.get_unchecked(k)) {
					j = k;
				}
				if self.data.get_unchecked(j).lt_prio(self.data.get_unchecked(i)) { break }
				swap_unchecked(self.data, i, j);
				i = j;
				j = (i << 1) + 1;
			}
		}
	}

	#[inline]
	fn sift_up(&mut self, idx: usize) {
		let mut i = idx;
		while i > 0 {
			let j = (i - 1) >> 1;
			unsafe {
				if self.data.get_unchecked(i).le_prio(&self.data[j]) { break }
				swap_unchecked(self.data, i, j);
			}
			i = j;
		}
	}

	#[inline(always)]
	fn heapify(&mut self) {
		let len = self.len;
		for i in (0..(len / 2)).rev() {
			self.sift_down(i, len)
		}
	}

	#[inline(always)]
	fn sort(&mut self) {
		for i in (1..self.len).rev() {
			swap_unchecked(self.data, 0, i);
			self.sift_down(0, i);
		}
		unsafe { from_raw_parts_mut(self.data.get_unchecked_mut(0), self.len).reverse() }
	}

	#[inline(always)]
	fn iter(&self) -> PrioQueueIter<T> {
		// TODO: change this to &mut Self when ICE goes away
		unsafe {
			transmute::<&PrioQueue<T>, &mut PrioQueue<T>>(self).sort();
			PrioQueueIter(from_raw_parts(self.data.get_unchecked(0), self.len))
		}
	}
}

container_debug!(PrioQueue; Priority);
