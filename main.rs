#![feature(core,debug_builders,filling_drop,lang_items,no_std,unsafe_destructor,unsafe_no_drop_flag)]
//#![no_std]
#[macro_use]
extern crate core;
use core::mem::uninitialized;
pub mod rust;
pub use rust::*;

#[derive(Debug)]
struct Person {
	name: &'static str,
	priority: u8
}
impl Priority for Person {
	type Prio = u8;
	fn priority(&self) -> u8 {
		255 - self.priority
	}
}

fn main() {
	let mut a: [Person; 10] = unsafe { uninitialized() };
	// note: unsafe because it is uninitialized
	let mut q = unsafe { PrioQueue::from_empty(&mut a) };
	q.add_or_panic(Person { name: "George",  priority: 7 },  "welp");
	q.add_or_panic(Person { name: "Carman",  priority: 3 },  "welp");
	q.add_or_panic(Person { name: "Jeff",    priority: 10 }, "welp");
	q.add_or_panic(Person { name: "Bob",     priority: 2 },  "welp");
	q.add_or_panic(Person { name: "Fred",    priority: 6 },  "welp");
	q.add_or_panic(Person { name: "Ian",     priority: 9 },  "welp");
	q.add_or_panic(Person { name: "Heather", priority: 8 },  "welp");
	q.add_or_panic(Person { name: "Doug",    priority: 4 },  "welp");
	q.add_or_panic(Person { name: "Eliza",   priority: 5 },  "welp");
	q.add_or_panic(Person { name: "Alice",   priority: 1 },  "welp");

	println!("full queue:");
	println!("{:#?}", q);
	q.forget(5); // removes multiple elements at once, slightly more efficently

	// note: on the OS, we'd use write!(cio, "{:#?}", ...)
	//       the :#? format indicates that we're going by position of arguments
	//       in the string (before the colon would be an index) and that
	//       the argument should be debug-formatted (?) and pretty-printed (#)
	println!("partially emptied queue:");
	println!("{:#?}", q);

	// here, we can println! on every step, but that ends up running heap sort
	// each time to display the queue in order
}

//#[lang = "stack_exhausted"] extern fn stack_exhausted() {}
//#[lang = "eh_personality"] extern fn eh_personality() {}
//#[lang = "panic_fmt"] fn panic_fmt() -> ! { loop {} }
