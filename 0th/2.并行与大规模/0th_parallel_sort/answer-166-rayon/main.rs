use text_io::read;
use rayon::prelude::*;

fn main() {
    let mut gen_A: u64 = read!();
    let mut gen_B: u64 = read!();
    let mut gen_C: u64 = read!();
    let n: usize = read!();

    let mut arr = vec![0; n as usize];
    for i in &mut arr {
        gen_A ^= gen_A << 31;
        gen_A ^= gen_A >> 17;
        gen_B ^= gen_B << 13; 
        gen_B ^= gen_B >> 5;
        gen_C += 1;
        gen_A ^=gen_B;
        gen_B ^= gen_C;
        *i = gen_A;
    }

    arr.par_sort_unstable();

    let ans: u64 = arr.par_iter()
        .enumerate()
        .map(|(i, &x)| x * i as u64)
        .reduce(|| 0, |x, y| x ^ y);
    println!("{}", ans)
}