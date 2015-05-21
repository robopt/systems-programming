#![allow(mutable_transmutes,unstable_feature)]
#![feature(core,debug_builders,filling_drop,lang_items,no_std,mutable_transmutes,unsafe_destructor,unsafe_no_drop_flag)]
//#![no_std]
#[macro_use]
extern crate core;
use core::mem::{forget, uninitialized};

#[macro_use]
pub mod rust;
pub use rust::*;

#[derive(Debug)]
struct Person {
	name: &'static str,
	prio: u8
}

impl Priority for Allocated<Person> {
	type Prio = u8;
	fn priority(&self) -> u8 {
		255 - self.prio
	}
}

impl Init for Person {
	fn init(&mut self) {
		static names: [&'static str; 10] = [
			"Alice",
			"Bob",
			"Carmen",
			"Doug",
			"Fred",
			"Gloria",
			"Hector",
			"Isabel",
			"Jeff",
			"Katherine"
		];
		static mut i: u8 = 0;
		// static mut: unsafe
		unsafe {
			self.name = names[i as usize];
			self.prio = i + 1;
			i = (i + 1) % 10;
		}
		println!("allocating {}", self.name)
	}
}

/*
// NOTE: lastest version of Rust broke this, so, i'll just comment out the
// destructor.
impl Drop for Person {
	fn drop(&mut self) {
		println!("deallocating {}", self.name)
	}
}
*/

impl_alloc!(
	Person,
	40, 40,
	"unable to allocate Person",
	"unable to deallocate Person"
);

fn main() {
	println!("INITIALISED ALLOCATOR");
	println!("{:#?}", Person::allocator());


	let mut a: [Allocated<Person>; 10] = unsafe { uninitialized() };
	// note: unsafe because it is uninitialized
	{
		let mut q = unsafe { PrioQueue::from_empty(&mut a) };
		q.add_or_panic(Person::allocator().alloc(), "welp");
		q.add_or_panic(Person::allocator().alloc(), "welp");
		q.add_or_panic(Person::allocator().alloc(), "welp");
		q.add_or_panic(Person::allocator().alloc(), "welp");
		q.add_or_panic(Person::allocator().alloc(), "welp");
		q.add_or_panic(Person::allocator().alloc(), "welp");
		q.add_or_panic(Person::allocator().alloc(), "welp");
		q.add_or_panic(Person::allocator().alloc(), "welp");
		q.add_or_panic(Person::allocator().alloc(), "welp");
		q.add_or_panic(Person::allocator().alloc(), "welp");

		println!("FULL QUEUE:");
		println!("{:#?}", q);
		println!("{:#?}", Person::allocator());
		q.forget(5); // removes multiple elements at once, slightly more efficently

		// note: on the OS, we'd use write!(cio, "{:#?}", ...)
		//       the :#? format indicates that we're going by position of arguments
		//       in the string (before the colon would be an index) and that
		//       the argument should be debug-formatted (?) and pretty-printed (#)
		println!("LESS FULL QUEUE:");
		println!("{:#?}", q);
		println!("{:#?}", Person::allocator());

		// here, we can println! on every step, but that ends up running heap sort
		// each time to display the queue in order

		// forget other 5 people
		q.forget(5);
	}
	unsafe { forget(a); }
	//
	// by now, all of the people have dropped out of scope
	println!("EVERYTHING DEALLOCATED:");
	println!("{:#?}", Person::allocator());
}

//#[lang = "stack_exhausted"] extern fn stack_exhausted() {}
//#[lang = "eh_personality"] extern fn eh_personality() {}
//#[lang = "panic_fmt"] fn panic_fmt() -> ! { loop {} }
