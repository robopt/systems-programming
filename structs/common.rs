use core::mem::{transmute, swap};

#[inline(always)]
pub fn swap_unchecked<T>(slice: &mut [T], i: usize, j: usize) {
	unsafe {
		let s0: *mut T = slice.get_unchecked_mut(0);
		let x = s0.offset(i as isize);
		let y = s0.offset(j as isize);
		swap::<T>(transmute(x), transmute(y));
	}
}
