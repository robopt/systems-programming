use core::mem::{transmute, swap};
use core::ptr::{copy_nonoverlapping, write};
use core::slice::{from_raw_parts, from_raw_parts_mut};
pub use core::slice::SliceExt;

pub trait SliceExt2 where Self: SliceExt {
	#[inline]
	unsafe fn offset_unchecked<'a>(&'a self, i: usize) -> &'a [Self::Item] {
		from_raw_parts(self.get_unchecked(i), self.len() - i)
	}

	#[inline]
	unsafe fn trunc_unchecked<'a>(&'a self, len: usize) -> &'a [Self::Item] {
		from_raw_parts(self.get_unchecked(0), len)
	}

	#[inline]
	unsafe fn slice_unchecked<'a>(&'a self, i: usize, j: usize) -> &'a [Self::Item] {
		from_raw_parts(self.get_unchecked(i), j - i)
	}

	#[inline]
	unsafe fn offset_unchecked_mut<'a>(&'a mut self, i: usize) -> &'a mut [Self::Item] {
		from_raw_parts_mut(self.get_unchecked_mut(i), self.len() - i)
	}

	#[inline]
	unsafe fn trunc_unchecked_mut<'a>(&'a mut self, len: usize) -> &'a mut [Self::Item] {
		from_raw_parts_mut(self.get_unchecked_mut(0), len)
	}

	#[inline]
	unsafe fn slice_unchecked_mut<'a>(&'a mut self, i: usize, j: usize) -> &'a mut [Self::Item] {
		from_raw_parts_mut(self.get_unchecked_mut(i), j - i)
	}

	#[inline]
	unsafe fn write_unchecked<'a>(&'a mut self, i: usize, value: Self::Item) {
		write(self.get_unchecked_mut(i), value)
	}

	#[inline]
	unsafe fn swap_unchecked<'a>(&'a mut self, i: usize, j: usize) {
		let p = self.as_ptr() as *mut Self::Item;
		let x = p.offset(i as isize);
		let y = p.offset(j as isize);
		swap::<Self::Item>(transmute(x), transmute(y))
	}

	#[inline]
	unsafe fn copy_unchecked<'a>(&'a mut self, i: usize, src: &'a [Self::Item]) {
		copy_nonoverlapping(src.get_unchecked(0), self.get_unchecked_mut(i), src.len())
	}
}

impl<T> SliceExt2 for [T] {}
