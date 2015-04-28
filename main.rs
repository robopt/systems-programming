#![feature(core,debug_builders,filling_drop,lang_items,no_std,unsafe_destructor,unsafe_no_drop_flag)]
#![no_std]
#[macro_use]
extern crate core;
pub mod rust;
pub use rust::*;
#[lang = "stack_exhausted"] extern fn stack_exhausted() {}
#[lang = "eh_personality"] extern fn eh_personality() {}
#[lang = "panic_fmt"] fn panic_fmt() -> ! { loop {} }
