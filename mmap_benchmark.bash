#!/bin/bash

# Filesystem partitions
EXT4_PARTITION="/dev/vdb"
F2FS_PARTITION="/dev/vda1"

# Mount points
MOUNT_EXT4="/mnt/ext4"
MOUNT_F2FS="/mnt/f2fs"

# Encryption directory
ENCRYPTED_DIR="encrypted"

# Test file
FILE_SIZE=$((16 * 1024 * 1024 * 1024))  # 16GB test file
TEST_FILE="testfile"
RESULT_FILE="/tmp/mmap_benchmark_results.txt"

# Ensure required tools are installed
if ! command -v fscrypt &>/dev/null || ! command -v dd &>/dev/null || ! command -v sync &>/dev/null || ! command -v mkfs.ext4 &>/dev/null || ! command -v mkfs.f2fs &>/dev/null; then
    echo "Required tools (fscrypt, dd, sync, mkfs.ext4, mkfs.f2fs) are missing. Install them first."
    exit 1
fi

# Function to create and mount filesystems
setup_filesystem() {
    local partition="$1"
    local fs_type="$2"
    local mount_point="$3"

    echo "Formatting $partition as $fs_type..."
    if [ "$fs_type" = "ext4" ]; then
        mkfs.ext4 -F "$partition"
	tune2fs -O encrypt "$partition"
    elif [ "$fs_type" = "f2fs" ]; then
        mkfs.f2fs -f "$partition"
	fsck.f2fs -O encrypt "$partition"
    else
        echo "Unsupported filesystem type!"
        exit 1
    fi

    echo "Mounting $partition to $mount_point..."
    mkdir -p "$mount_point"
    mount "$partition" "$mount_point"

    echo "Enabling fscrypt on $mount_point..."
    fscrypt setup "$mount_point" --quiet
}

# Function to create an encrypted directory
setup_fscrypt() {
    local mount_point="$1"
    local enc_dir="$mount_point/$ENCRYPTED_DIR"

    echo "Setting up encrypted directory at $enc_dir..."
    mkdir -p "$enc_dir"
    echo -n "password" | fscrypt encrypt "$enc_dir" --source=custom_passphrase --quiet --name=mykey
}

# Function to create a test file
create_test_file() {
    local path="$1/$TEST_FILE"
    
    echo "Creating test file at $path..."
    dd if=/dev/urandom of="$path" bs=1M count=$((FILE_SIZE / 1024 / 1024)) status=none
    sync
}

# Function to run mmap benchmark
benchmark_mmap() {
    local file_path="$1"
    local output_label="$2"

    echo "Benchmarking mmap read for $output_label..."
    ./mmap_benchmark "$file_path" >> "$RESULT_FILE"
}

# Clear previous results
echo "Filesystem Benchmark Results" > "$RESULT_FILE"

# Setup filesystems
setup_filesystem "$EXT4_PARTITION" "ext4" "$MOUNT_EXT4"
setup_filesystem "$F2FS_PARTITION" "f2fs" "$MOUNT_F2FS"

# Run tests
for FS in "ext4" "f2fs"; do
    MOUNT_VAR="MOUNT_${FS^^}"
    MOUNT_POINT="${!MOUNT_VAR}"

    if [ ! -d "$MOUNT_POINT" ]; then
        echo "Skipping $FS (mount not found: $MOUNT_POINT)"
        continue
    fi

    echo "Testing $FS..."

    # Without encryption
    create_test_file "$MOUNT_POINT"
    benchmark_mmap "$MOUNT_POINT/$TEST_FILE" "$FS (no encryption)"
    rm -f "$MOUNT_POINT/$TEST_FILE"

    # With encryption
    setup_fscrypt "$MOUNT_POINT"
    create_test_file "$MOUNT_POINT/$ENCRYPTED_DIR"
    benchmark_mmap "$MOUNT_POINT/$ENCRYPTED_DIR/$TEST_FILE" "$FS (encrypted)"
    rm -f "$MOUNT_POINT/$ENCRYPTED_DIR/$TEST_FILE"
done

# Cleanup
echo "Unmounting and cleaning up..."
umount "$MOUNT_EXT4"
umount "$MOUNT_F2FS"
rm -rf "$MOUNT_EXT4" "$MOUNT_F2FS"

echo "Benchmark complete. Results saved in $RESULT_FILE."
