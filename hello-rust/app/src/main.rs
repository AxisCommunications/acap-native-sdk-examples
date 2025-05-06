#![forbid(unsafe_code)]
//! A simple "Hello, World!"-application
//!
//! This app demonstrates the effect of printing to stdout and stderr.

fn main() {
    eprintln!("Hello stderr!");
    println!("Hello stdout!");
}
