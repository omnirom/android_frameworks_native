fn main() {
    // Anything with cargo is NDK only. If you want to access anything else, use Soong.
    println!("cargo::rustc-cfg=android_ndk");
}
