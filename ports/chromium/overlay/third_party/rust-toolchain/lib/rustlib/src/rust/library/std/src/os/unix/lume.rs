pub mod raw {
    #[stable(feature = "raw_ext", since = "1.1.0")]
    pub type pthread_t = libc::pthread_t;
    #[stable(feature = "raw_ext", since = "1.1.0")]
    pub type blkcnt_t = libc::blkcnt_t;
    #[stable(feature = "raw_ext", since = "1.1.0")]
    pub type blksize_t = libc::blksize_t;
    #[stable(feature = "raw_ext", since = "1.1.0")]
    pub type dev_t = libc::dev_t;
    #[stable(feature = "raw_ext", since = "1.1.0")]
    pub type ino_t = libc::ino_t;
    #[stable(feature = "raw_ext", since = "1.1.0")]
    pub type mode_t = libc::mode_t;
    #[stable(feature = "raw_ext", since = "1.1.0")]
    pub type nlink_t = libc::nlink_t;
    #[stable(feature = "raw_ext", since = "1.1.0")]
    pub type off_t = libc::off_t;
    #[stable(feature = "raw_ext", since = "1.1.0")]
    pub type time_t = libc::time_t;
}

pub mod fs {
    use crate::fs::Metadata;
    use crate::sys::AsInner;

    pub trait MetadataExt {
        fn st_dev(&self) -> u64;
        fn st_ino(&self) -> u64;
        fn st_mode(&self) -> u32;
        fn st_nlink(&self) -> u64;
        fn st_uid(&self) -> u32;
        fn st_gid(&self) -> u32;
        fn st_rdev(&self) -> u64;
        fn st_size(&self) -> u64;
        fn st_atime(&self) -> i64;
        fn st_atime_nsec(&self) -> i64;
        fn st_mtime(&self) -> i64;
        fn st_mtime_nsec(&self) -> i64;
        fn st_ctime(&self) -> i64;
        fn st_ctime_nsec(&self) -> i64;
        fn st_blksize(&self) -> u64;
        fn st_blocks(&self) -> u64;
    }

    impl MetadataExt for Metadata {
        fn st_dev(&self) -> u64 {
            self.as_inner().as_inner().st_dev as u64
        }
        fn st_ino(&self) -> u64 {
            self.as_inner().as_inner().st_ino as u64
        }
        fn st_mode(&self) -> u32 {
            self.as_inner().as_inner().st_mode as u32
        }
        fn st_nlink(&self) -> u64 {
            self.as_inner().as_inner().st_nlink as u64
        }
        fn st_uid(&self) -> u32 {
            self.as_inner().as_inner().st_uid as u32
        }
        fn st_gid(&self) -> u32 {
            self.as_inner().as_inner().st_gid as u32
        }
        fn st_rdev(&self) -> u64 {
            self.as_inner().as_inner().st_rdev as u64
        }
        fn st_size(&self) -> u64 {
            self.as_inner().as_inner().st_size as u64
        }
        fn st_atime(&self) -> i64 {
            self.as_inner().as_inner().st_atim.tv_sec as i64
        }
        fn st_atime_nsec(&self) -> i64 {
            self.as_inner().as_inner().st_atim.tv_nsec as i64
        }
        fn st_mtime(&self) -> i64 {
            self.as_inner().as_inner().st_mtim.tv_sec as i64
        }
        fn st_mtime_nsec(&self) -> i64 {
            self.as_inner().as_inner().st_mtim.tv_nsec as i64
        }
        fn st_ctime(&self) -> i64 {
            self.as_inner().as_inner().st_ctim.tv_sec as i64
        }
        fn st_ctime_nsec(&self) -> i64 {
            self.as_inner().as_inner().st_ctim.tv_nsec as i64
        }
        fn st_blksize(&self) -> u64 {
            self.as_inner().as_inner().st_blksize as u64
        }
        fn st_blocks(&self) -> u64 {
            self.as_inner().as_inner().st_blocks as u64
        }
    }
}
