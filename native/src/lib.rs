use std::{mem, ptr};

use napi::bindgen_prelude::*;
use napi::{NapiRaw, NapiValue};
use napi_derive::napi;

pub struct Value {
    env: sys::napi_env,
    value: sys::napi_value,
}

impl NapiRaw for Value {
    unsafe fn raw(&self) -> sys::napi_value {
        self.value
    }
}

impl NapiValue for Value {
    unsafe fn from_raw(env: sys::napi_env, value: sys::napi_value) -> Result<Self> {
        Ok(Self { env, value })
    }

    unsafe fn from_raw_unchecked(env: sys::napi_env, value: sys::napi_value) -> Self {
        Self { env, value }
    }
}

#[napi(js_name = "wordLength")]
pub fn word_length(n: Value) -> Result<u32> {
    let mut word_count = 0usize;

    check_status!(unsafe {
        sys::napi_get_value_bigint_words(
            n.env,
            n.value,
            ptr::null_mut(),
            &mut word_count,
            ptr::null_mut(),
        )
    })?;

    Ok(word_count as u32)
}

fn to_bytes_le_inner(n: Value, buf: &mut [u8]) -> Result<u32> {
    let dst = buf.as_mut_ptr().cast::<u64>();

    let mut sign_bit = 0;
    let mut word_count = buf.len() / 8;

    if is_aligned(dst) {
        check_status!(unsafe {
            sys::napi_get_value_bigint_words(n.env, n.value, &mut sign_bit, &mut word_count, dst)
        })?;
    } else {
        let mut tmp_buf = Vec::with_capacity(word_count);

        unsafe {
            check_status!(sys::napi_get_value_bigint_words(
                n.env,
                n.value,
                &mut sign_bit,
                &mut word_count,
                tmp_buf.as_mut_ptr(),
            ))?;

            ptr::copy_nonoverlapping(tmp_buf.as_ptr().cast::<u8>(), dst.cast::<u8>(), buf.len());
        }
    }

    Ok(match word_count {
        0 => 0,
        _ => {
            let tail = unsafe { *(dst.add(word_count - 1)) };
            (word_count * 8) as u32 - (tail.leading_zeros() / 8)
        }
    })
}

#[napi(js_name = "toBytesLE")]
pub fn to_bytes_le(n: Value, mut buf: Uint8Array) -> Result<u32> {
    to_bytes_le_inner(n, &mut buf)
}

#[napi(js_name = "toBytesBE")]
pub fn to_bytes_be(n: Value, mut buf: Uint8Array) -> Result<u32> {
    let len = to_bytes_le_inner(n, &mut buf)?;
    buf[..(len as usize)].reverse();
    Ok(len)
}

pub enum ToBigIntLe {
    Borrowed { words: *const u64, word_count: usize },
    Owned { words: Vec<u64> },
}

impl ToNapiValue for ToBigIntLe {
    unsafe fn to_napi_value(env: sys::napi_env, val: Self) -> Result<sys::napi_value> {
        let (words, word_count) = match val {
            ToBigIntLe::Borrowed { words, word_count } => (words, word_count),
            ToBigIntLe::Owned { words } => (words.as_ptr(), words.len()),
        };

        let mut value = ptr::null_mut();
        check_status!(sys::napi_create_bigint_words(env, 0, word_count, words, &mut value))?;
        Ok(value)
    }
}

#[napi(js_name = "fromBytesLE")]
pub fn from_bytes_le(buf: Uint8Array) -> ToBigIntLe {
    let src = buf.as_ptr();
    let len = buf.len();

    let word_count = len / 8;
    let remainder = len % 8;

    if is_aligned(src.cast::<u64>()) && remainder == 0 {
        ToBigIntLe::Borrowed { words: src.cast::<u64>(), word_count }
    } else {
        let word_count = word_count + (remainder != 0) as usize;
        let mut words = vec![0; word_count];

        let dst = words.as_mut_ptr().cast::<u8>();

        unsafe { ptr::copy_nonoverlapping(src, dst, len) };

        ToBigIntLe::Owned { words }
    }
}

#[napi(js_name = "fromBytesBE")]
pub fn from_bytes_be(buf: Uint8Array) -> BigInt {
    let src = buf.as_ptr();
    let len = buf.len();

    let word_count = len / 8;
    let remainder = len % 8;

    let word_count = word_count + (remainder != 0) as usize;
    let mut words = vec![0; word_count];

    let dst = words.as_mut_ptr().cast::<u8>();

    unsafe { ptr::copy_nonoverlapping(src, dst, len) };

    BigInt { sign_bit: false, words }
}

fn is_aligned<T>(ptr: *const T) -> bool {
    (ptr as usize) % mem::align_of::<T>() == 0
}
