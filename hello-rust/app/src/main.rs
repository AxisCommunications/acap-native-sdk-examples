#![forbid(unsafe_code)]
//! A simple hello world application
//!
//! This app demonstrates:
//! - The effect of printing to stdout and stderr.
//! - How to configure logging and the effect of logging at various levels.

use log::{debug, error, info, trace, warn};

fn main() {
    eprintln!("Hello stderr!");
    println!("Hello stdout!");
    acap_logging::init_logger();
    trace!("Hello trace!");
    debug!("Hello debug!");
    info!("Hello info!");
    warn!("Hello warn!");
    error!("Hello error!");
}
